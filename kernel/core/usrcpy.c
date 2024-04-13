#include <bits/errno.h>
#include <lib/string.h>
#include <lib/printk.h>
#include <lib/stdint.h>
#include <lib/types.h>

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

void swapi64(i64 *a0, i64 *a1) {
    i64 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}

void swapi32(i32 *a0, i32 *a1) {
    i32 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}

void swapi16(i16 *a0, i16 *a1) {
    i16 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}

void swapi8(i8 *a0, i8 *a1) {
    i8 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}

void swapu64(u64 *a0, u64 *a1) {
    u64 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}

void swapu32(u32 *a0, u32 *a1) {
    u32 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}

void swapu16(u16 *a0, u16 *a1) {
    u16 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}

void swapu8(u8 *a0, u8 *a1) {
    u8 tmp = 0;
    assert(a0 && a1, "Invalid inputs");

    tmp = *a1;
    *a1 = *a0;
    *a0 = tmp;
}