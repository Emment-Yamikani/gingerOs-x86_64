#include <dev/dev.h>


static int cpudev_init(void) {
    return 0;
}

MODULE_INIT(cpu, NULL, cpudev_init, NULL);