#include <stdlib.h>
#include <ginger/syscall.h>

void abort(void) {
	sys_exit(-1);
	__builtin_unreachable();
}
