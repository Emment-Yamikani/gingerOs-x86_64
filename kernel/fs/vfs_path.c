#include <fs/fs.h>
#include <fs/path.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <lib/printk.h>
#include <sys/system.h>

#include <lib/stdint.h>
#include <bits/errno.h>
#include <lib/stddef.h>
#include <fs/fs.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <fs/path.h>

int verify_path(const char *path) {
    int     err     = -EINVAL;
    size_t  length  = 0, component_length = 0;

    if (!path || !*path) {
        return err; // NULL or empty path is invalid
    }

    for (; *path; ++path, ++length) {
        // Null character is not allowed
        if (*path == '\0') {
            return err;
        }

        // Slash (/) as part of a filename component is invalid in Unix
        if (*path == '/') {
            component_length = 0; // Reset for new directory level
            continue;
        }

        // Track the length of individual path components
        component_length++;
        if (component_length > NAME_MAX) {
            return -ENAMETOOLONG; // Component name too long
        }
    }

    // Exceeding PATH_MAX is not allowed.
    if (length > PATH_MAX) {
        return -ENAMETOOLONG;
    }

    return 0; // Path is valid
}

void path_free(vfspath_t *path) {
    if (path == NULL)
        return;
    
    if (path->absolute)
        kfree(path->absolute);
    
    if (path->tokenized)
        tokens_free(path->tokenized);
    
    kfree(path);
}

int path_get_lasttoken(const char *pathname, char **ltok) {
    int         err         = 0;
    size_t      index       = 0;
    size_t      ntok        = 0;
    char        *last       = NULL;
    char        **tokenized = NULL;
    char        **canonical = NULL;

    if (ltok == NULL)
        return -EINVAL;

    if ((err = verify_path(pathname)))
        return err;
    
    if ((err = canonicalize_path(pathname, &ntok, &canonical, &last)))
        return err;

    if (NULL == (tokenized = (char **)kcalloc(ntok + 1, sizeof (char *)))) {
        tokens_free(canonical);
        return -ENOMEM;
    }

    foreach (token, canonical) {
        if (string_eq(".", token)) {
            kfree(token);
            continue;
        }
    
        if (string_eq("..", token)) {
            if (index > 0)
                index--;
            kfree(token);
            continue;
        }
        tokenized[index++] = token;
    }

    if (last != NULL){
        if (NULL == (*ltok = strdup(last)))
            err = -ENOMEM;
    } else if (last == NULL && string_eq(pathname, "/")) {
        if (NULL == (*ltok = strdup("/")))
            err = -ENOMEM;
    } else err = -EINVAL;

    kfree(canonical);
    tokens_free(tokenized);
    return err;
}

int vfspath_parse(const char *pathname, int flags, vfspath_t **rp) {
    int         err         = 0;
    int         isdir       = 0;
    size_t      index       = 0;
    size_t      pathlen     = 0;
    size_t      pathtoks    = 0;
    char        *tmppath    = NULL;
    vfspath_t   *vfspath    = NULL;
    char        *absolute   = NULL;
    char        *lasttoken  = NULL;
    char        **tokenized = NULL;
    char         **canonical= NULL;

    if (pathname == NULL || *pathname == '\0')
        return -ENOTNAM;

    if ((err = verify_path(pathname)))
        return err;

    if (*pathname == '/')
        flags |= PATH_ABSOLUTE;

    if (flags & PATH_VERIFY_ONLY)
        return 0;
    else if (rp == NULL)
        return -EINVAL;

    if (NULL == (tmppath = strdup(pathname)))
        return -ENOMEM;
    
    if (tmppath[strlen(tmppath) - 1] == '/')
        isdir = 1;

    if ((err = canonicalize_path(tmppath, &pathtoks, &canonical, &lasttoken)))
        goto error;

    if (NULL == (tokenized = kcalloc(pathtoks + 1, sizeof (char *)))) {
        err = -ENOMEM;
        goto error;
    }

    foreach (token, canonical) {
        if (string_eq(".", token)) {
            kfree(token);
            continue;
        }
    
        if (string_eq("..", token)) {
            if (index > 0)
                index--;
            kfree(token);
            continue;
        }
        tokenized[index++] = lasttoken = token;
    }

    pathtoks = index;

    // reset index.
    index = 0;

    if ((flags & PATH_NOABS) == 0) { // if NOABS not specified, concatinate path.
        foreach(token, tokenized) {
            size_t  nchar       = 0;
            size_t  tokenlen    = 0;
            char    *tmp        = NULL;
            char    seperator   = '/';

            /// get and check the length of each token
            /// ensure token length is <= MAXFNAME.
            if ((tokenlen = strlen(token)) > MAXFNAME) {
                err = -ENAMETOOLONG;
                goto error1;
            }

            // add token length to pathlen + 1 for '\0'(str terminator).
            pathlen += tokenlen + 1;

            // test for end of path components.
            if (token == lasttoken) {
                // last token is not a directory so add a '\0' at end instead.
                if (!isdir) {
                    pathlen--;
                    seperator = '\0';
                }
            } else {
                // Not yet at end of path, so just add a 1 to account for '/'.
                pathlen++;
            }

            if (NULL == (tmp = krealloc(absolute, pathlen))) {
                err = -ENOMEM;
                goto error1;
            }

            absolute    = tmp;
            nchar       = tokenlen + (seperator ? 2 : 1);
            // printk("pathlen: %d, tlen: %d, idx: %d, nchar: %d\n", pathlen, tokenlen, index, nchar);
            snprintf(absolute + index, nchar, "%s%c", token, seperator);
            index += nchar - 1;
        }

        if (absolute == NULL) {
            if (NULL == (absolute = strdup("/"))) {
                err = -ENOMEM;
                goto error1;
            }
        }
    }

    if (NULL == (vfspath = (vfspath_t *)kcalloc(1, sizeof *vfspath))) {
        err = -ENOMEM;
        goto error1;
    }

    vfspath_setflags(vfspath, flags | (isdir ? PATH_ISDIR : 0));

    if (flags & PATH_NOTOKENIZED) {
        tokens_free(tokenized);
    } else {
        vfspath->tokencount = pathtoks;
        vfspath->tokenized  = tokenized;
    }

    vfspath->absolute   = absolute;
    vfspath->lasttoken  = lasttoken;

    kfree(tmppath);
    kfree(canonical);

    *rp = vfspath;
    return 0;

error1:
    if (tmppath)
        kfree(tmppath);

    if (tokenized)
        tokens_free(tokenized);

    if (canonical)
        kfree(canonical);

    if (absolute)
        kfree(absolute);

    if (vfspath)
        kfree(vfspath);

    return err;
error:
    if (tmppath)
        kfree(tmppath);
    
    if (tokenized)
        kfree(tokenized);

    if (canonical)
        tokens_free(canonical);    
    
    if (absolute)
        kfree(absolute);
    
    if (vfspath)
        kfree(vfspath);
    
    return err;
}