#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include  <fs/dentry.h>

typedef struct vfspath_t {
    int     flags;      // flags specifying how to parse the given path.
    dentry_t *dentry;   // if lookup succeeded, this is the dentry we were searching for.
    char    *absolute;  // absolute path, i.e from the root dir.
    char    *lasttoken; // last token in the path e.g, 'isa' is the last token in the path: '/dev/bus/isa'
    size_t  tokencount; // number of tokens in parsed path.
    char    **tokenized;// absolute path from the root dir in a tokenized format.
    dentry_t *directory;// if lookup failed, this is the directory lookup ended at.
} vfspath_t;

#define PATH_NOABS         0x01 //do not return absolute path.
#define PATH_NOTOKENIZED   0x02 //do not return tokenized path
#define PATH_VERIFY_ONLY   0x04 //Just do a path verification.
#define PATH_NOLAST_TOK    0x08 //do not return last token of path.
#define PATH_ISDIR         0x10 //is a directory implied by the pathname?


void path_free(vfspath_t *path);
int verify_path(const char *__path);
int parse_path(const char *path, const char *cwd,
                 int flags, vfspath_t **pref);

int path_get_lasttoken(const char *path, char **ltok);