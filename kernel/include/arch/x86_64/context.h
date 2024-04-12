#pragma once

#include <lib/stdint.h>
#include <lib/types.h>

typedef struct {
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

    u64 trapno;
    u64 err_code;
    
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
}tf_t;

#define x86_64_tf_isuser(tf)    ({((tf)->err_code & 0x4) ? 1 : 0; })

typedef struct {
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 rbx;
    u64 rbp;
    u64 rip;
} context_t;

// No. of registers in caller-callee context.
#define NREGCTX ((sizeof (context_t)) / sizeof (uintptr_t))

extern void trapret(void);
extern void signal_exec(void);
extern void swtch(context_t **old, context_t *new);