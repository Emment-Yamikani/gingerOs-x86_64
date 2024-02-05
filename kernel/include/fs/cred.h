#pragma once

#include <lib/types.h>

typedef struct cred_t {
    char    *c_cwd;
    char    *c_root;
    uid_t   c_uid;
    uid_t   c_euid;
    uid_t   c_suid;
    gid_t   c_gid;
    gid_t   c_egid;
    gid_t   c_sgid;
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
