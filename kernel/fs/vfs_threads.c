#include <sys/thread.h>
#include <ginger/jiffies.h>

int resource_claimer(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    
    loop() {

    }
    
    __builtin_unreachable();
} BUILTIN_THREAD(resource_claimer, resource_claimer, NULL);


void cache_cleaner(void) {
    BUILTIN_THREAD_ANOUNCE(__func__);
    
    loop() {

    }
    
    __builtin_unreachable();
} BUILTIN_THREAD(cache_cleaner, cache_cleaner, NULL);