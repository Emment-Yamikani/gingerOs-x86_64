#pragma once

#include <arch/x86_64/context.h>

void do_syscall(tf_t *tf);

#define SYSCALL_PUTC    0

extern void sys_putc(int c);

extern void exit(int exit_code);
