#pragma once

#include <lib/stdint.h>
#include <lib/types.h>

#define x86_64_tf_isuser(tf)    ({((tf)->errno & 0x4) ? 1 : 0; })

typedef struct __context_t {
    context_t *link;
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

enum {
    CXT_LINK,
#define CTX_LINK CTX_LINK
    CTX_R15, 
#define CTX_R15 CTX_R15
    CTX_R14, 
#define CTX_R14 CTX_R14
    CTX_R13, 
#define CTX_R13 CTX_R13
    CTX_R12, 
#define CTX_R12 CTX_R12
    CTX_R11, 
#define CTX_R11 CTX_R11
    CTX_RBX, 
#define CTX_RBX CTX_RBX
    CTX_RBP, 
#define CTX_RBP CTX_RBP
    CTX_RIP, 
#define CTX_RIP CTX_RIP
};

extern void trapret(void);
extern void signal_exec(void);
extern void swtch(context_t **old, context_t *new);
extern void context_switch(context_t **pcontext);