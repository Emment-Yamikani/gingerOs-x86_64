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
 * @param __cwd is the working directory into which path resides.
 * @param __abspath is the returned absolute path.
 * @param __abspath_tokens is the returned absolute path in a canonical form.
 * @param __last_token is the last token in the resolved path.
 * 
 * @returns (int)0 on success and non-zero on failure.
 * 
*/
int parse_path(const char *path, const char *__cwd,
               char **__abspath, char ***__abspath_tokens,
               char **__last_token, int *pisdir) {
    int is_dir = 0;
    int err = -ENOTNAM;
    size_t tmp_cwdlen = 0, tmp_pathlen = 0;
    size_t tmp_abslen = 0, ntoken = 0, tok_i = 0;
    char *tmp_path = NULL, *last_token = NULL, **token_buffer = NULL;
    char *cwd = NULL, **tokens = NULL, *abspath = NULL, *tmp_cwd = NULL;

    if (!path || !*path)
        goto error;

    if (!__cwd || (*path == '/'))
        cwd = "/";
    else if (!*__cwd)
        goto error;
    else if (*__cwd != '/')
    {
        err = -ENOMEM;
        tmp_cwdlen = strlen(__cwd);
        if (!(tmp_cwd = cwd = kmalloc((tmp_cwdlen + 2))))
            goto error;
        cwd[0] = '/';
        strncpy((cwd + 1), __cwd, tmp_cwdlen);
        cwd[tmp_cwdlen + 1] = '\0';
    }
    else
        cwd = (char *)__cwd;

    if ((err = verify_path(cwd)))
        goto error;

    if ((err = verify_path(path)))
        goto error;

    tmp_cwdlen = strlen(cwd);
    tmp_pathlen = strlen(path);
    tmp_abslen = tmp_cwdlen + tmp_pathlen + 2;

    if (path[tmp_pathlen - 1] == '/')
        is_dir = 1;

    err = -ENOMEM;
    if (!(tmp_path = kmalloc(tmp_abslen)))
        goto error;

    strncpy(tmp_path, cwd, tmp_cwdlen);
    tmp_path[tmp_cwdlen] = '/';
    strncpy((tmp_path + tmp_cwdlen + 1), path, tmp_pathlen);
    tmp_path[tmp_abslen - 1] = '\0';

    if (!(tokens = canonicalize_path(tmp_path, &ntoken, NULL)))
        goto error;

    if (!(token_buffer = kcalloc((ntoken + 1), sizeof(char *))))
        goto error;

    foreach (token, tokens)
    {
        if (!compare_strings(".", token))
            continue;
        if (!compare_strings("..", token))
        {
            if (tok_i > 0)
                tok_i--;
            continue;
        }
        token_buffer[tok_i++] = last_token = token;
    }

    token_buffer[tok_i] = NULL;

    err = -ENOMEM;

    if (!(abspath = kmalloc(2)))
        goto error;

    *abspath = '/';
    abspath[1] = '\0';
    tmp_abslen = 1;
    tmp_pathlen = 0;

    foreach (token, token_buffer)
    {
        tmp_abslen += (tmp_pathlen = strlen(token)) + 1;

        err = -ENAMETOOLONG;
        if (tmp_pathlen > MAXFNAME)
            goto error;

        err = -ENOMEM;

        if (!(abspath = krealloc(abspath, tmp_abslen)))
            goto error;

        strncpy(abspath + (tmp_abslen - (tmp_pathlen + 1)), token, tmp_pathlen);

        if (token != last_token)
            abspath[(tmp_abslen - 1)] = '/';
        else
            abspath[(tmp_abslen - 1)] = '\0';
    }

    err = -ENOMEM;
    if (__last_token) {
        if (last_token) {
            if (!(*__last_token = strdup(last_token)))
                goto error;
        } else if (!(*__last_token = strdup("/")))
            goto error;
    }

    kfree(tokens);
    kfree(tmp_path);

    if (tmp_cwd)
        kfree(tmp_cwd);

    if (__abspath)
        *__abspath = abspath;
    else
        kfree(__abspath);

    if (__abspath_tokens)
        *__abspath_tokens = token_buffer;
    else
        tokens_free(token_buffer); /// @FIXME: may cause kmalloc to fail.

    if (pisdir)
        *pisdir = is_dir;
    return 0;
error:
    if (tokens)
        kfree(tokens);
    if (tmp_cwd)
        kfree(tmp_cwd);
    if (tmp_path)
        kfree(tmp_path);
    if (abspath)
        kfree(abspath);
    if (token_buffer)
        tokens_free(token_buffer);
    return err;
}

int path_get_lasttoken(const char *path, char **last_token) {
    return parse_path(path, NULL, NULL, NULL, last_token, NULL);
}

int path_process(const char *pathname, const char *cwd, int flags, path_t **pref) {
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
    path_t  *path       = NULL;
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
        size_t tokenlen = 0;
        
        err = -ENAMETOOLONG;
        if ((tokenlen = strlen(token)) > MAXFNAME)
            goto error;
        
        err = -ENOMEM;
        abslen += tokenlen;
        if (NULL == (tmp = krealloc(abs, abslen + 1)))
            goto error;
        
        strncpy((abs = tmp) + (abslen - tokenlen), token, tokenlen);
        if (token != tokens[ntok - 1])
            abs[abslen] = '/';
        else
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

    if (flags & PATH_NOLAST_TOK)
        kfree(last_token);
    else path->lasttoken = last_token;

    if (flags & PATH_NOTOKENIZED)
        tokens_free(tokens);
    else {path->tokenized = tokens, path->tokencount = tok_i;};


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