#include <arch/cpu.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <sys/_signal.h>
#include <mm/kalloc.h>
#include <sys/sysproc.h>
#include <arch/thread.h>
#include <arch/signal.h>

int raise(int signo) {
    (void)signo;
    return -ENOSYS;
}

int pause(void) {
    return -ENOSYS;
}

unsigned long alarm(unsigned long sec) {
    (void)sec;

    return -ENOSYS;
}