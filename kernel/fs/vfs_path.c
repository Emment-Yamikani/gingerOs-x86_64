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


/**
 * @brief Parse the pathname of a file and return an absolute path.
 * __abspath, __abspath_tokens and __last_token may be NULL.
 * The returned __abspath, __abspath_tokens and __last_token should be kfree-ed after use.
 * 
 * @param path is the path to be parsed.
 * @param cwd is the working directory into which path resides.
 * @param pref: is a pointer to type vfspath_t *, this is where the following,
 *      according to flags will be returned:
 *      1. absolute     : is the returned absolute path.
 *      2. tokens       : is the returned absolute path in a canonical form.
 *      3. last_token   : is the last token in the resolved path.
 * 
 * @returns (int)0 on success and non-zero on failure.
 * 
*/
int parse_path(const char *pathname, const char *cwd, int flags, vfspath_t **pref) {
    int     err         = 0;
    size_t  ntok        = 0;
    size_t  tok_i       = 0;
    int     isdir       = 0;
    size_t  abslen      = 0;
    size_t  cwdlen      = 0;
    size_t  tmplen      = 0;
    size_t  pathlen     = 0;
    char    *__cwd      = NULL;
    char    *abs        = NULL;
    char    *tmppath    = NULL;
    char    *__last_tok = NULL;
    char    *last_token = NULL;
    vfspath_t  *path    = NULL;
    char    **tokens    = NULL;
    char    **tmptokens = NULL;

    if (pathname == NULL || *pathname == '\0')
        return -ENOTNAM;

    if (*pathname == '/' || (cwd == NULL))
        cwd = "/";

    if (*cwd == '\0')
        return -ENOTNAM;

    if ((err = verify_path(cwd)))
        return err;

    if ((err = verify_path(pathname)))
        return err;

    if (flags & PATH_VERIFY_ONLY)
        return 0;
    else if (NULL == pref)
        return -EINVAL;

    /**
     * If current working directory isn't starting with '/',
     * convert it so it begins with a '/'.
     */
    if (*cwd != '/') {
        tmplen = strlen(cwd);
        if (NULL == (__cwd = kmalloc(tmplen + 2)))
            return -ENOMEM;
        __cwd[0] = '/';
        strncpy(__cwd + 1, cwd, tmplen);
        __cwd[tmplen + 1] = '\0';
        cwd = __cwd;
    } else {
        flags |= PATH_ABSOLUTE;
    }

    cwdlen = strlen(cwd);
    pathlen = strlen(pathname);
    isdir = pathname[pathlen] == '/' ? 1 : 0;

    err = -ENOMEM;

    if (NULL == (tmppath = kmalloc(cwdlen + pathlen + 2)))
        goto error;

    strncpy(tmppath, cwd, cwdlen);
    tmppath[cwdlen] = '/';
    strncpy(tmppath + cwdlen + 1, pathname, pathlen);
    tmppath[cwdlen + pathlen + 1] = '\0';

    if ((err= canonicalize_path(tmppath, &ntok, &tmptokens, NULL)))
        goto error;

    if (NULL == (tokens = kcalloc(ntok + 1, sizeof(char *))))
        goto error;
    
    foreach (token, tmptokens) {
        if (!compare_strings(".", token))
            continue;
        if (!combine_strings("..", token)) {
            if (tok_i > 0)
                tok_i--;
            continue;
        }
        tokens[tok_i++] = __last_tok = token;
    }

    tokens[tok_i] = NULL;

    if (NULL == (abs = kmalloc(2)))
        goto error;
    
    abslen = 1;
    abs[0] = '/';
    abs[1] = '\0';

    foreach (token, tokens) {
        char *tmp = NULL;
        err = -ENAMETOOLONG;
        size_t tokenlen = 0;
        
        abslen += tokenlen = strlen(token);
        if (tokenlen > MAXFNAME)
            goto error;
        
        err = -ENOMEM;
        if (NULL == (tmp = krealloc(abs, abslen + 1)))
            goto error;
        
        strncpy((abs = tmp) + (abslen - tokenlen), token, tokenlen);

        if (token != tokens[tok_i - 1]) {
            abs[abslen] = '/';
            abslen++;
        } else {
            abs[abslen] = '\0';
        }
    }

    err = -ENOMEM;
    if (__last_tok) {
        if (NULL == (last_token = strdup(__last_tok)))
            goto error;
    } else if (NULL == (last_token = strdup("/")))
        goto error;
    
    if (NULL == (path = kcalloc(1, sizeof *path)))
        goto error;

    kfree(tmppath);
    kfree(tmptokens);
    
    vfspath_setflags(path, flags | (isdir ? PATH_ISDIR : 0));

    if (flags & PATH_NOABS)
        kfree(abs);
    else
        path->absolute = abs;

    if (flags & PATH_NOTOKENIZED) {
        tokens_free(tokens);
    } else {
        path->tokencount = tok_i;
        path->tokenized = tokens;
    };

    if (flags & PATH_NOLAST_TOK)
        kfree(last_token);
    else
        path->lasttoken = last_token;

    *pref = path;
    return 0;
error:
    if (last_token)
        kfree(last_token);
    if (abs)
        kfree(abs);
    if (tokens)
        kfree(tokens);
    if (tmptokens)
        tokens_free(tmptokens);
    if (tmppath)
        kfree(tmppath);
    if (__cwd)
        kfree(__cwd);
    return err;
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
    int err = 0;
    vfspath_t *path = NULL;

    if (ltok == NULL)
        return -EINVAL;

    if ((err = parse_path(pathname, NULL, PATH_NOABS| PATH_NOTOKENIZED, &path)))
        return err;
    
    if (NULL == (*ltok = strdup(path->lasttoken))) {
        err = -ENOMEM;
    }

    path_free(path);

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