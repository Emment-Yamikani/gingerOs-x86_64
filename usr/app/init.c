#include <ginger/unistd.h>

void handler(int);

void main(void) {

    signal(SIGALRM, handler);
    
    alarm(2);

    loop();
}


void handler(int signo) {
    printf("%s:%d: caught signal(%d)\n", __FILE__, __LINE__, signo);
}