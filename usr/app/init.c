#include <ginger/unistd.h>

void main(void) {
    int err = 0;
    int pipes[2];

    assert_msg((err = pipe(pipes)) == 0,
        "failed to create a pipe, err: %d\n", err);

    printf("pipe[%d:%d], err: %d\n", pipes[0], pipes[1], err);

    loop();
}