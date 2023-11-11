#pragma once

#include <lib/types.h>

typedef struct cred_t {
    char    *c_cwd, *c_root;
    uid_t   c_uid, c_euid, c_suid;
    gid_t   c_gid, c_egid, c_sgid;
    mode_t  c_umask;
} cred_t;

#define UIO_DEFAULT() ((cred_t){\
    .c_cwd  = "/",              \
    .c_root = "/",              \
    .c_uid  = 0,                \
    .c_euid = 0,                \
    .c_egid = 0,                \
    .c_gid  = 0,                \
    .c_suid = 0,                \
    .c_sgid = 0,                \
})
