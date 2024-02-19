#include <bits/errno.h>
#include <sys/_utsname.h>
#include <sys/system.h>

int uname (utsname_t *utsname __unused) {
    return -ENOSYS;
}