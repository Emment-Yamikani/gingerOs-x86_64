#include <dev/dev.h>


static int cpudev_init(void) {
    return 0;
}

MODULE_INIT(cpu, cpudev_init, NULL, NULL);