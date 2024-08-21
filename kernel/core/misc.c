#include <core/types.h>

void *fast_memset(void *dest, int c, usize len) {
    u8 *ptr = (u8 *)dest;
    u64 fill_value = (u8)c;

    // Create a fill value with the byte value repeated across the word
    fill_value |= fill_value << 8;
    fill_value |= fill_value << 16;
    fill_value |= fill_value << 32;

    // Align ptr to the nearest 64-bit boundary
    while (len > 0 && ((u64)ptr & (sizeof(u64) - 1)) != 0) {
        *ptr++ = (u8)c;
        len--;
    }

    // Fill by 64-bit chunks
    while (len >= sizeof(u64)) {
        *(u64 *)ptr = fill_value;
        ptr += sizeof(u64);
        len -= sizeof(u64);
    }

    // Fill by 32-bit chunks
    if (len >= sizeof(u32)) {
        u32 fill_32 = (u32)fill_value;
        while (len >= sizeof(u32)) {
            *(u32 *)ptr = fill_32;
            ptr += sizeof(u32);
            len -= sizeof(u32);
        }
    }

    // Fill the remaining bytes
    while (len--) {
        *ptr++ = (u8)c;
    }

    return dest;
}

void *bzero(void *ptr, usize size) {
    return fast_memset(ptr, 0, size);
}