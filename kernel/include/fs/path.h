#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>

typedef struct {
    int     flags;
    char    *absolute;
    char    *lasttoken;
    size_t  tokencount;
    char    **tokenized;
} path_t;

#define PATH_NOABS         0x01 //do not return absolute path.
#define PATH_NOTOKENIZED   0x02 //do not return tokenized path
#define PATH_VERIFY_ONLY   0x04 //Just do a path verification.
#define PATH_NOLAST_TOK    0x08 //do not return last token of path.
#define PATH_ISDIR         0x10 //is a directory implied by the pathname?


void path_free(path_t *path);
int verify_path(const char *__path);
int parse_path(const char *path, const char *cwd,
                 int flags, path_t **pref);

int path_get_lasttoken(const char *path, char **ltok);