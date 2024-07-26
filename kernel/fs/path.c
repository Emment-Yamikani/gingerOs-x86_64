#include <lib/stdint.h>
#include <bits/errno.h>
#include <lib/stddef.h>
#include <fs/fs.h>
#include <mm/kalloc.h>
#include <lib/string.h>
#include <fs/path.h>

int verify_path(const char *path) {
    int err = -ENOTNAM;

    if (!path)
        goto error;

    if (!*path)
        goto error;

    for (; *path; ++path) {
        if ((*path == '\n') ||
            (*path == '\t') ||
            (*path < (char)' ') ||
            (*path == (char)'\\') ||
            (*path == (char)0x7f) ||
            (*path == (char)0x81) ||
            (*path == (char)0x8D) ||
            (*path == (char)0x9D) ||
            (*path == (char)0xA0) ||
            (*path == (char)0xAD) ||
            (*path == '\r')) // NOT NECESSARY. REALLY!
            goto error;
    }

    return 0;
error:
    return err;
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
    vfspath_t  *path       = NULL;
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

    if (NULL == (tmptokens = canonicalize_path(tmppath, &ntok, NULL)))
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
        } else
            abs[abslen] = '\0';
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
    
    path->flags = flags | (isdir ? PATH_ISDIR : 0);

    if (flags & PATH_NOABS)
        kfree(abs);
    else path->absolute = abs;

    if (flags & PATH_NOTOKENIZED)
        tokens_free(tokens);
    else {
        path->tokencount = tok_i;
        path->tokenized = tokens;
    };

    if (flags & PATH_NOLAST_TOK)
        kfree(last_token);
    else path->lasttoken = last_token;

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
    
    if (path->lasttoken)
        kfree(path->lasttoken);
    
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