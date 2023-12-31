#pragma once

#include <lib/stdint.h>

typedef struct {
    uintptr_t fs;
    uintptr_t ds;

    uintptr_t r15;
    uintptr_t r14;
    uintptr_t r13;
    uintptr_t r12;
    uintptr_t r11;
    uintptr_t r10;
    uintptr_t r9;
    uintptr_t r8;

    uintptr_t rbp;
    uintptr_t rsi;
    uintptr_t rdi;
    uintptr_t rdx;
    uintptr_t rcx;
    uintptr_t rbx;
    uintptr_t rax;

    uintptr_t trapno;
    uintptr_t err_code;
    
    uintptr_t rip;
    uintptr_t cs;
    uintptr_t rflags;
    uintptr_t rsp;
    uintptr_t ss;
}tf_t;

#define x86_64_tf_isuser(tf)    ({((tf)->err_code & 0x4) ? 1 : 0; })

typedef struct {
    uintptr_t r15;
    uintptr_t r14;
    uintptr_t r13;
    uintptr_t r12;
    uintptr_t r11;
    uintptr_t r10;
    uintptr_t r9;
    uintptr_t rbx;
    uintptr_t rbp;
    uintptr_t rip;
} context_t;

extern void trapret(void);
extern void signal_exec(void);
extern void swtch(context_t **old, context_t *new);