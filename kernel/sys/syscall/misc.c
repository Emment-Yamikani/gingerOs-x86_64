#include <bits/errno.h>
#include <sys/_utsname.h>
#include <sys/system.h>
#include <lib/printk.h>

int uname (utsname_t *utsname __unused) {
    if (utsname == NULL)
        return -EFAULT;

    snprintf(utsname->machine, _UTSNAME_LENGTH, "ginger-os-test kit");
    snprintf(utsname->nodename, _UTSNAME_LENGTH, "n/a");
    snprintf(utsname->release, _UTSNAME_LENGTH, "0.0");
    snprintf(utsname->sysname, _UTSNAME_LENGTH, "ginger Os");
    snprintf(utsname->version, _UTSNAME_LENGTH, "0.0");

    return 0;
}


int isatty(int fd __unused) {
    return -ENOSYS;
}
