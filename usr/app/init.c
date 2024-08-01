#include <ginger/unistd.h>

void main(void) {
    int     err     = 0;
    int     fd      = 0;
    mode_t  mode    = 0777;
    devid_t dev     = mkdev(5, 3);
    char    buf[]   = "Hello /tmp/foo/bar";
    
    if ((err = mkdir("/tmp/foo/", mode)) < 0)
        panic("Failed to mkdir, error: %d\n", err);

    printf("opening file\n");

    chdir("/tmp/foo");

    if ((err = fd = open("bar", O_CREAT | O_RDWR | O_TRUNC, mode)) < 0)
        panic("Failed to open, error: %d\n", err);
    
    close(fd);
    
    if ((err = fd = open("/tmp/foo/bar", O_RDWR, mode)) < 0)
        panic("Failed to open, error: %d\n", err);
    
    if ((err = write(fd, buf, sizeof buf)) < 0)
        panic("Failed to write, error: %d\n", err);
    lseek(fd, 0, SEEK_SET);
    memset(buf, 0, sizeof buf);
    if ((err = read(fd, buf, sizeof buf)) < 0)
        panic("Failed to read, error: %d\n", err);
    if ((err = close(fd)) < 0)
        panic("Failed to close, error: %d\n", err);

    printf("DATA: %s\n", buf);

    if ((err = mknod("/dev/console", mode, dev)))
        panic("Failed to creat device node: err: %d", err);

    loop();
}