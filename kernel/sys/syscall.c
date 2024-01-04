#include <bits/errno.h>
#include <lib/printk.h>
#include <sys/syscall.h>
#include <arch/x86_64/context.h>

size_t (*syscall[])() = {
    [SYSCALL_PUTC] = (void *)sys_putc,
};

static int sys_syscall_ni(tf_t *tf) {
    printk("syscall(%d) not implemented\n", tf->rax);
    return -ENOSYS;
}

void do_syscall(tf_t *tf) {
    if (tf == NULL)
        return;
    
    if (tf->rax >= NELEM(syscall))
        tf->rax = sys_syscall_ni(tf);
    else if ((long)tf->rax < 0 || !syscall[tf->rax])
        tf->rax = sys_syscall_ni(tf);
    else
        tf->rax = (syscall[tf->rax])(tf->rdi,
        tf->rsi, tf->rdx, tf->rcx, tf->r8, tf->r9, tf->rsp);
}

void sys_putc(int c) {
    printk("%c", c);
}