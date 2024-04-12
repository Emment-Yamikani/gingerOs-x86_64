#pragma once

#include <arch/x86_64/context.h>
#include <lib/types.h>
#include <sys/_signal.h>
#include <sys/system.h>

typedef struct __mcontext_t {
    u64 rsvd[NREGCTX];
    // general purpose registers.
#if defined (__x86_64__)
    u64 fs;
    u64 ds;

    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;

    u64 rbp;
    u64 rsi;
    u64 rdi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;

    // TODO: include cr2 and other necessary registers.

    u64 trapno;
    u64 errno;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
#endif // #if defined (__x86_64__)
} __packed mcontext_t;


typedef struct __ucontext_t {
    ucontext_t *uc_link;    /* pointer to context resumed when */
                            /* this context returns */
    sigset_t    uc_sigmask; /* signals blocked when this context */
                            /* is active */
    uc_stack_t  uc_stack;   /* stack used by this context */
    mcontext_t  uc_mcontext;/* machine-specific representation of */
                            /* saved context */
} ucontext_t;