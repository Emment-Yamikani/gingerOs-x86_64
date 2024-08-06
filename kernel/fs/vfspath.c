#include <fs/fs.h>
#include <fs/path.h>
#include <bits/errno.h>
#include <sys/thread.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <lib/printk.h>
#include <sys/system.h>

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
    char         **canonocal= NULL;

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

    if ((err = canonicalize_path(tmppath, &pathtoks, &canonocal, &lasttoken)))
        goto error;

    if (NULL == (tokenized = kcalloc(pathtoks + 1, sizeof (char *)))) {
        err = -ENOMEM;
        goto error;
    }

    foreach (token, canonocal) {
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
    kfree(canonocal);

    *rp = vfspath;
    return 0;

error1:
    if (tmppath)
        kfree(tmppath);

    if (tokenized)
        tokens_free(tokenized);

    if (canonocal)
        kfree(canonocal);

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

    if (canonocal)
        tokens_free(canonocal);    
    
    if (absolute)
        kfree(absolute);
    
    if (vfspath)
        kfree(vfspath);
    
    return err;
}