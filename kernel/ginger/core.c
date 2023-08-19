#include <lib/types.h>
#include <sys/thread.h>
#include <sys/sleep.h>

void start_others(void);

void A(void) {
    loop();
}

void B(void) {
    loop();
}
BUILTIN_THREAD(B, B, NULL);

void C(void) {
    loop();
}
BUILTIN_THREAD(C, C, NULL);