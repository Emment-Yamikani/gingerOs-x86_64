#include "../include/api.h"

void thrd(void) {
    printf("Greetings from thread[%d:%d]\n", sys_getpid(), sys_gettid());
    loop();
    sys_thread_exit(0);
}

long tx = 0;
spinlock_t *lk = SPINLOCK_NEW();

int main(int argc, char const*argv[]) {
    (void)argc, (void)argv;
    printf("%p, thread: %d, locked: %d, guard: %d\n", &lk, lk->thread, lk->locked, lk->guard);

    for (tid_t tid = 0; tid < 5; printf("tid: %d running...\n", tid))
        sys_thread_create(&tid, NULL, (thread_entry_t)thrd, NULL);

    printf("Main thread\nGoodbye!!!\n[%d:%d:%d]\n", lk->guard, lk->locked, lk->thread);
    return 0xDEADBEEF;
}