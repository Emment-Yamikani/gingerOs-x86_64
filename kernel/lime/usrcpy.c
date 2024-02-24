#include <bits/errno.h>
#include <lib/string.h>
#include <lib/printk.h>
#include <lib/stdint.h>


int copy_to_user(void *udst, void *ksrc, size_t size) {
    if (udst == NULL || ksrc == NULL)
        return -EFAULT;

    if (size == 0)
        return -ERANGE;

    if (memcpy(udst, ksrc, size) != udst)
        return -EINVAL;

    return 0;
}

int copy_from_user(void *kdst, void * usrc, size_t size) {
    if (kdst == NULL || usrc == NULL)
        return -EFAULT;

    if (size == 0)
        return -ERANGE;

    if (memcpy(kdst, usrc, size) != kdst)
        return -EINVAL;

    return 0;
}