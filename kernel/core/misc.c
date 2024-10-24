#include <core/misc.h>
#include <lib/printk.h>
#include <lib/string.h>

void bzero(void *b, usize sz) {
    assert(b, "No block\n");
    memset(b, 0, sz);
}