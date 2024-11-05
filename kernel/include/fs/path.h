#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <fs/dentry.h>
#include <sys/system.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

typedef struct vfspath_t {
    int         flags;          // flags specifying how to parse the given path.
    dentry_t    *dentry;        // if lookup succeeded, this is the dentry we were searching for.
    char        *absolute;      // absolute path, i.e from the root dir.
    char        *lasttoken;     // last token in the path e.g, 'isa' is the last token in the path: '/dev/bus/isa'
    size_t      tok_index;      // last token _index
    size_t      tokencount;     // number of tokens in parsed path.
    char        **tokenized;    // absolute path from the root dir in a tokenized format.
    dentry_t    *directory;     // if lookup failed, this is the directory lookup ended at.
    char        *token;         // last token to be looked up.
} vfspath_t;

#define PATH_NOABS          BS(0)   // Do not return absolute path.
#define PATH_NOTOKENIZED    BS(1)   // Do not return tokenized path
#define PATH_VERIFY_ONLY    BS(2)   // Just do a path verification.
#define PATH_NOLAST_TOK     BS(3)   // Do not return last token of path.
#define PATH_ISDIR          BS(4)   // Is a directory implied by the pathname?
#define PATH_TOKEN_ISLAST   BS(5)   // path->token is the last element in the path.
#define PATH_ABSOLUTE       BS(6)   // path is relative.

#define vfspath_assert(path)                ({ assert((path), "No vfspath struct specified."); })
#define vfspath_setflags(path, __flags__)   ({ vfspath_assert(path); (path)->flags |= (__flags__); })
#define vfspath_testflags(path, __flags__)  ({ vfspath_assert(path); (path)->flags & (__flags__); })
#define vfspath_maskflags(path, __flags__)  ({ vfspath_assert(path); (path)->flags &= ~(__flags__); })


#define vfspath_set_noabs(path)             ({ vfspath_setflags(path, PATH_NOABS); })
#define vfspath_set_nottokenized(path)      ({ vfspath_setflags(path, PATH_NOTOKENIZED); })
#define vfspath_set_verify_only(path)       ({ vfspath_setflags(path, PATH_VERIFY_ONLY); })
#define vfspath_set__nolasttoken(path)      ({ vfspath_setflags(path, PATH_NOLAST_TOK); })
#define vfspath_set_dir(path)               ({ vfspath_setflags(path, PATH_ISDIR); })
#define vfspath_set_lasttoken(path)         ({ vfspath_setflags(path, PATH_TOKEN_ISLAST); })
#define vfspath_set_absolute(path)          ({ vfspath_setflags(path, PATH_ABSOLUTE); })

#define vfspath_isnoabs(path)               ({ vfspath_testflags(path, PATH_NOABS); })
#define vfspath_isnottokenized(path)        ({ vfspath_testflags(path, PATH_NOTOKENIZED); })
#define vfspath_isverify_only(path)         ({ vfspath_testflags(path, PATH_VERIFY_ONLY); })
#define vfspath_is_nolasttoken(path)        ({ vfspath_testflags(path, PATH_NOLAST_TOK); })
#define vfspath_isdir(path)                 ({ vfspath_testflags(path, PATH_ISDIR); })
#define vfspath_islasttoken(path)           ({ vfspath_testflags(path, PATH_TOKEN_ISLAST); })
#define vfspath_isabsolute(path)            ({ vfspath_testflags(path, PATH_ABSOLUTE); })

#define vfspath_mask_noabs(path)            ({ vfspath_maskflags(path, PATH_NOABS); })
#define vfspath_mask_nottokenized(path)     ({ vfspath_maskflags(path, PATH_NOTOKENIZED); })
#define vfspath_mask_verify_only(path)      ({ vfspath_maskflags(path, PATH_VERIFY_ONLY); })
#define vfspath_mask__nolasttoken(path)     ({ vfspath_maskflags(path, PATH_NOLAST_TOK); })
#define vfspath_mask_dir(path)              ({ vfspath_maskflags(path, PATH_ISDIR); })
#define vfspath_mask_lasttoken(path)        ({ vfspath_maskflags(path, PATH_TOKEN_ISLAST); })
#define vfspath_mask_absolute(path)         ({ vfspath_maskflags(path, PATH_ABSOLUTE); })

void path_free(vfspath_t *path);
int verify_path(const char *__path);
int path_get_lasttoken(const char *path, char **ltok);
int vfspath_parse(const char *pathname, int flags, vfspath_t **rp);
int parse_path(const char *path, const char *cwd, int flags, vfspath_t **pref);