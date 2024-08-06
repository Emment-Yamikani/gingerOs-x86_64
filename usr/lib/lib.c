#include <api.h>

dev_t mkdev(int major, int minor) {
    return ((devid_t)(((devid_t)(minor) << 8) & 0xff00) | ((devid_t)(major) & 0xff));
}