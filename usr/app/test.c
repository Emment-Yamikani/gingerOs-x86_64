#include "../include/api.h"
#include <sys/mman.h>

void fun(void) {
    loop()
        sys_thread_yield();
}

int main(int argc, char const*argv[]) {
    (void)argc, (void)argv;
    tid_t tid = 0;
    
    printf("Hello, World!\n");
    
    while (tid < 10)
        sys_thread_create(&tid, NULL, (thread_entry_t)fun, NULL);

    char *pa = sys_mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);


    *pa = 'F';
    printf("Wrote pa: %c\n", *pa);
    sys_mprotect(pa, 0x1000, PROT_READ);
    *pa = 'X';

    printf("Wrote pa: %c\n", *pa);

    return 0xDEADBEEF;
}