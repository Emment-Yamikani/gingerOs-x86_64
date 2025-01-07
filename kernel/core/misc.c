#include <core/misc.h>
#include <core/assert.h>
#include <lib/string.h>

void bzero(void *b, usize sz) {
    assert(b, "No block\n");
    memset(b, 0, sz);
}