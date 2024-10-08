#include <lib/printk.h>
#include <sys/sched.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <ds/queue.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <arch/cpu.h>
#include <ginger/jiffies.h>
#include <arch/lapic.h>
#include <sys/proc.h>

int sched_init(void) {
    int err = 0;

    cpu->ncli = 0;
    current = NULL;
    cpu->intena = 0;
    
    memset(&ready_queue, 0, sizeof ready_queue);

    for (size_t i = 0; i < NELEM(ready_queue.level); ++i) {
        if ((err = queue_alloc(&ready_queue.level[i].queue)))
            goto error;
        ready_queue.level[i].quatum = ms_TO_jiffies(20);
    }

    return 0;
error:
    for (size_t i = 0; i < NELEM(ready_queue.level); ++i) {
        if (ready_queue.level[i].queue) {
            queue_free(ready_queue.level[i].queue);
            ready_queue.level->queue = NULL;
        }
    }
    return err;
}

void sched(void) {
    long ncli = 0, intena = 0;

    pushcli();
    ncli    = cpu->ncli;
    intena  = cpu->intena;
    current_assert_locked();

    if (current_issetpark() && current_isisleep()) {
        if (current_issetwake()) {
            current_mask_park_wake();
            popcli();
            return;
        }
    }

    // thread_chain_lock_release(current);
    context_switch(&current->t_arch.t_ctx);
    // thread_chain_lock_acquire(current);

    current_assert_locked();
    cpu->ncli   = ncli;
    cpu->intena = intena;
    popcli();
}

void sched_yield(void) {
    current_lock();
    current->t_state = T_READY;
    sched();
    current_unlock();
}

int sched_park(thread_t *thread) {
    int             prior       = 0;
    int             affini      = 0;
    level_t         *lvl        = NULL;
    cpu_t           *processor  = NULL;
    thread_sched_t  *tsched     = NULL;
    int             core        = getcpuid();

    if (thread == NULL)
        return -EINVAL;
    
    thread_assert_locked(thread);

    if (thread_isembryo(thread)) {
        return sched_putembryo(thread);
    }

    tsched  = &thread->t_sched;
    prior   = tsched->ts_priority;
    affini  = tsched->ts_affinity.type;

    if (affini == HARD_AFFINITY) {
        /// test cpus for affinity
        /// if thread has hard affinity scheduling enabled.
        for (core = 0; core < cpu_online(); core++) {
            if (BTEST(tsched->ts_affinity.cpu_set, core))
                break;
        }

        if (core >= cpu_online()) {
            core = getcpuid();
            tsched->ts_affinity.cpu_set |= BS(core);
        }
    }

    processor = tsched->ts_processor ? tsched->ts_processor : cpus[core];

    if (processor == NULL)
        processor = cpu;
    
    tsched->ts_processor = processor;
    lvl = &processor->queueq.level[SCHED_LEVEL(prior)];
    return thread_enqueue(lvl->queue, thread, NULL);
}

thread_t *sched_next(void) {
    level_t  *lvl    = NULL;
    thread_t *thread = NULL;

    if (NULL == (thread = sched_getembryo()))
        goto self;
    
    thread_enter_state(thread, T_READY);
    assert(!sched_park(thread), "Failed to park\n");
    thread_unlock(thread);
self:
    thread = NULL;
    for (int i = 0; i < NLEVELS; ++i) {
        lvl = &ready_queue.level[i];
        queue_lock(lvl->queue);
        thread = thread_dequeue(lvl->queue);
        queue_unlock(lvl->queue);

        if (thread == NULL)
            continue;
        
        thread->t_sched.ts_timeslice = lvl->quatum;
        break;
    }
    return thread;
}

static void sched_self_destruct(void) {
    int err = 0;
    current_assert_locked();

    /**
     * pushcli() to make sure interrupts are not
     * reenabled when we call current_unlock() below
     * and at the end of this routine.
     * TRUST ME: its a very nasty BUG: i spent a days trying to locate it.
    */
    pushcli();
    current_unlock();
    
    // TODO: increment running thread count for a thread group.

    current_lock();

    if (current_isdetached()) {
        current_unlock();
        thread_detach(current);
        current_lock();
    }

    if ((err = sched_putzombie(current))) {
        panic("??FAILED TO ZOMBIE CPU%d:TID:%d, error: %d\n",
        getcpuid(), thread_self(), err);
    }

    current_unlock();
}

__noreturn void schedule(void) {
    int             err     = 0;
    uintptr_t       pdbr    = 0;
    jiffies_t       before  = 0;
    mmap_t          *mmap   = NULL;
    arch_thread_t   *arch   = NULL;
    thread_t        *thread = NULL;
    thread_sched_t  *tsched = NULL;

    if ((err = sched_init())) {
        panic(
            "cpu%d failed to initialize "
            "scheduling queues. err: %d\n",
            getcpuid(), err
        );
    }

    loop() {
        cpu->ncli   = 0;
        cpu->intena = 0;
        current     = NULL;

        sti(); // start hardware interrupts here

        if (NULL == (thread = sched_next())) {
            /// TODO: make cpu core enter an idle state,
            /// to reduce queue contentions in sched_next().
            /// instead of hlt() and cpu_pause().
            hlt();
            cpu_pause();
            continue;
        }

        cli(); // temporarily stop hardware interrupts

        // set the new thread to run as 'current'
        current = thread;
        
        /// ensure current thred is locked,
        /// this will be implicity unlocked,
        /// in arch_thread_start() or in sched().
        current_assert_locked();

        if (current_iszombie()  ||
            current_iskilled()  ||
            current_isstopped() ||
            current_isterminated()) {

            if (current_isstopped()) {
                pushcli();
                /// TODO: Hope this code is not executed \'ever!\'
                panic(
                    "[\e[0;04mWARNING\e[0m] "
                    "???Hmmm this will call "
                    "sched(), do you know the"
                    " implications of doing this???\n"
                );
                thread_stop(current, sched_stopq);
                continue;
            }

            current_enter_state(T_TERMINATED);
            if (current_iskilled())
                current->t_exit = -EINTR;
            sched_self_destruct();
            continue;
        }

        mmap    = current->t_mmap;
        arch    = &current->t_arch;
        tsched  = &current->t_sched;

        // get the current time of scheduling.
        tsched->ts_last_sched = jiffies_TO_s(before = jiffies_get());

        if (mmap) {
            mmap_lock(mmap);
            mmap_focus(mmap, &pdbr);
            mmap_unlock(mmap);
            // Make sure a thread running in a seperate address space
            // to that of the kernel must have it's kernel stack pointer
            // set up in the tss.
            // TODO: use tss.ist in later versions of this code.
            err = arch_thread_setkstack(&current->t_arch);
            assert_msg(err == 0, "Kernel stack was not set "
                "for user thread, errno = %d\n", err);
        }


        // Context switch to the new thread.
        // This will, depending of the stack frame,
        // unlock the current thread struct in sched()
        // if the thread is returning from a call to sched()
        // or arch_thread_start() if this is the first
        // time the thread is being run.
        //swtch(&cpu->ctx, arch->t_ctx);

        // printk("0: cpu%d, tid: %d, "
        //     "ctx: %p, ctx->link: %p\n",
        //     getcpuid(), thread_gettid(current), arch->t_ctx,
        //     arch->t_ctx->link
        // );

        context_switch(&arch->t_ctx);
        
        // printk("1: cpu%d, tid: %d, "
        //     "ctx: %p, ctx->link: %p\n",
        //     getcpuid(), thread_gettid(current), arch->t_ctx,
        //     arch->t_ctx->link
        // );

        // Do no allow current to return to schedule() without acquiring
        // a lock on itself.
        current_assert_locked();
    
        // get the time thread returned execution to the scheduler.
        tsched->ts_cpu_time += jiffies_TO_s(jiffies_get() - before);
        
        pushcli();
        if (current_iskilled()) {
            current_enter_state(T_TERMINATED);
            current->t_exit = -EINTR;
        }

        switch (current_getstate()) {
        case T_EMBRYO:
            panic(
                "??\e[0;04mT_EMBRYO\e[0m"
                " was allowed to run\n"
            );
            break;
        case T_READY:
            sched_park(current);
            current_unlock();
            break;
        case T_ISLEEP:
        case T_USLEEP:
        case T_STOPPED:
            current_unlock();
            break;
        case T_TERMINATED:
            sched_self_destruct();
            break;
        default:
            panic(
                "??cpu%d tid:%d called sched()"
                " while thread is in %s state\n",
                getcpuid(), thread_self(),
                t_states[current_getstate()]
            );
        }
    }
}