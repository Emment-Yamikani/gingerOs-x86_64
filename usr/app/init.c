#include <ginger/unistd.h>

int pi[2];

void th(void) {
    sleep(10);
    write(pi[1], "&y", 3);
}

void main(void) {
    int err = 0;
    int in = 0, out = 0;
    char buf[100];

    thread_create(NULL, NULL, (thread_entry_t)th, NULL);

    assert_msg((err = pipe(pi)) == 0,
        "failed to create a pipe, err: %d\n", err);

    in  = pi[0];
    out = pi[1];

    printf("writting to pipe\n");
    if ((err = write(out, "Hello, World :) b", 17)) < 0)
        panic("%s:%d: error: %d\n", __FILE__, __LINE__, err);

    if ((err = read(in, buf, 20)) < 0)
        panic("%s:%d: error: %d\n", __FILE__, __LINE__, err);
    
    printf("PIPE: %s\n", buf);
    loop();
}