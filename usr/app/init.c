#include <ginger/unistd.h>
#include <api.h>

tid_t tid = 0;

void main(void) {
    if (tid == 0) {
        tid = thread_self();
        loop() {
            thread_create(NULL, NULL, (void *)main, NULL);
        }
    }
}