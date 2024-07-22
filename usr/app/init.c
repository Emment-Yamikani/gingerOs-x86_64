#include <ginger/unistd.h>

int fd[3];

void write_msg(const char *msg) {
    printf("Thread[%d:%d]: %s", gettid(), getpid(), msg);
}

void *producer(void *arg __unused) {
    int err = 0;
    char buf[8192];

    fd[2] = open("/ramfs/makefile", O_RDONLY, 0);

    err = read(fd[2], buf, sizeof buf);
    assert_msg(err >= -1,
        "Failed to 'read', error: %d\n", err
    );

    assert_msg((err = write(fd[1], buf, sizeof buf)) > 0,
        "Failed to 'write', error: %d\n", err
    );

    write_msg("Producer is done\n");
    
    return NULL;
}

void main(void) {
    int err = 0;
    char buf[129];

    assert(pipe(fd) == 0, "Failed to create pipe");

    thread_create(NULL, NULL, producer, NULL);

    for (isize sz = 0; sz <= 8192; sz += 128) {
            err = read(fd[0], buf, 128);
        assert_msg(err >= -1,
            "Failed to 'read', error: %d\n", err
        );
        printf(buf);
        memset(buf, 0, sizeof buf);
    }

    loop();
}