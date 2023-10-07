#include <lib/string.h>
#include <lib/stddef.h>
#include <mm/kalloc.h>
#include <lib/stdint.h>
#include <lib/limits.h>
#include <lib/ctype.h>
#include <sys/system.h>
#include <bits/errno.h>


#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(X) (((X)-ONES) & ~(X)&HIGHS)

#define BITOP(A, B, OP) \
    ((A)[(size_t)(B) / (8 * sizeof *(A))] OP(size_t) 1 << ((size_t)(B) % (8 * sizeof *(A))))

int memcmp(const void *vl, const void *vr, size_t n)
{
    const uint8_t *l = vl;
    const uint8_t *r = vr;
    for (; n && *l == *r; n--, l++, r++)
        ;
    return n ? *l - *r : 0;
}

void *memchr(const void *src, int c, size_t n)
{
    const uint8_t *s = src;
    c = (uint8_t)c;
    for (; ((uintptr_t)s & (ALIGN - 1)) && n && *s != c; s++, n--)
        ;
    if (n && *s != c)
    {
        const size_t *w;
        size_t k = ONES * c;
        for (w = (const void *)s; n >= sizeof(size_t) && !HASZERO(*w ^ k); w++, n -= sizeof(size_t))
            ;
        for (s = (const void *)w; n && *s != c; s++, n--)
            ;
    }
    return n ? (void *)s : 0;
}

void *memrchr(const void *m, int c, size_t n)
{
    const uint8_t *s = m;
    c = (uint8_t)c;
    while (n--)
    {
        if (s[n] == c)
        {
            return (void *)(s + n);
        }
    }
    return 0;
}

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size)
{
    uint8_t *dst = (uint8_t *)dstptr;
    const uint8_t *src = (const uint8_t *)srcptr;
    for (size_t i = 0; i < size; i++)
        dst[i] = src[i];
    return dstptr;
}

void *memcpy32(void *restrict dstptr, const void *restrict srcptr, size_t n)
{
    uint32_t *dst = (uint32_t *)dstptr;
    const uint32_t *src = (const uint32_t *)srcptr;
    for (size_t i = 0; i < n; i++)
        dst[i] = src[i];
    return dstptr;
}

void *memset(void *bufptr, int value, size_t size)
{
    uint8_t *buf = (uint8_t *)bufptr;
    for (size_t i = 0; i < size; i++)
        buf[i] = (uint8_t)value;
    return bufptr;
}

void *memsetw(void *bufptr, int value, size_t size)
{
    uint16_t *buf = (uint16_t *)bufptr;
    for (size_t i = 0; i < size; i++)
        buf[i] = (uint16_t)value;
    return bufptr;
}

int strcmp(const char *l, const char *r)
{
    for (; *l == *r && *l; l++, r++)
        ;
    return *(uint8_t *)l - *(uint8_t *)r;
}

int strcoll(const char *s1, const char *s2)
{
    return strcmp(s1, s2); /* TODO locales */
}

size_t strlen(const char *s)
{
    const char *a = s;
    const size_t *w;
    for (; (uintptr_t)s % ALIGN; s++)
    {
        if (!*s)
        {
            return s - a;
        }
    }
    for (w = (const void *)s; !HASZERO(*w); w++)
        ;
    for (s = (const void *)w; *s; s++)
        ;
    return s - a;
}

char *stpcpy(char *restrict d, const char *restrict s)
{
    size_t *wd;
    const size_t *ws;

    if ((uintptr_t)s % ALIGN == (uintptr_t)d % ALIGN)
    {
        for (; (uintptr_t)s % ALIGN; s++, d++)
        {
            if (!(*d = *s))
            {
                return d;
            }
        }
        wd = (void *)d;
        ws = (const void *)s;
        for (; !HASZERO(*ws); *wd++ = *ws++)
            ;
        d = (void *)wd;
        s = (const void *)ws;
    }

    for (; (*d = *s); s++, d++)
        ;

    return d;
}

char *strcpy(char *restrict dest, const char *restrict src)
{
    char *out = dest;
    for (; (*dest = *src); src++, dest++)
        ;
    return out;
}

size_t strspn(const char *s, const char *c)
{
    const char *a = s;
    size_t byteset[32 / sizeof(size_t)] = {0};

    if (!c[0])
    {
        return 0;
    }
    if (!c[1])
    {
        for (; *s == *c; s++)
            ;
        return s - a;
    }

    for (; *c && BITOP(byteset, *(uint8_t *)c, |=); c++)
        ;
    for (; *s && BITOP(byteset, *(uint8_t *)s, &); s++)
        ;

    return s - a;
}

char *strchrnul(const char *s, int c)
{
    size_t *w;
    size_t k;

    c = (uint8_t)c;
    if (!c)
    {
        return (char *)s + strlen(s);
    }

    for (; (uintptr_t)s % ALIGN; s++)
    {
        if (!*s || *(uint8_t *)s == c)
        {
            return (char *)s;
        }
    }

    k = ONES * c;
    for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w ^ k); w++)
        ;
    for (s = (void *)w; *s && *(uint8_t *)s != c; s++)
        ;
    return (char *)s;
}

char *strchr(const char *s, int c)
{
    char *r = strchrnul(s, c);
    return *(uint8_t *)r == (uint8_t)c ? r : 0;
}

char *strrchr(const char *s, int c)
{
    return memrchr(s, c, strlen(s) + 1);
}

size_t strcspn(const char *s, const char *c)
{
    const char *a = s;
    if (c[0] && c[1])
    {
        size_t byteset[32 / sizeof(size_t)] = {0};
        for (; *c && BITOP(byteset, *(uint8_t *)c, |=); c++)
            ;
        for (; *s && !BITOP(byteset, *(uint8_t *)s, &); s++)
            ;
        return s - a;
    }
    return strchrnul(s, *c) - a;
}

char *strpbrk(const char *s, const char *b)
{
    s += strcspn(s, b);
    return *s ? (char *)s : 0;
}

static char *strstr_2b(const uint8_t *h, const uint8_t *n)
{
    uint16_t nw = n[0] << 8 | n[1];
    uint16_t hw = h[0] << 8 | h[1];
    for (h++; *h && hw != nw; hw = hw << 8 | *++h)
        ;
    return *h ? (char *)h - 1 : 0;
}

static char *strstr_3b(const uint8_t *h, const uint8_t *n)
{
    uint32_t nw = n[0] << 24 | n[1] << 16 | n[2] << 8;
    uint32_t hw = h[0] << 24 | h[1] << 16 | h[2] << 8;
    for (h += 2; *h && hw != nw; hw = (hw | *++h) << 8)
        ;
    return *h ? (char *)h - 2 : 0;
}

static char *strstr_4b(const uint8_t *h, const uint8_t *n)
{
    uint32_t nw = n[0] << 24 | n[1] << 16 | n[2] << 8 | n[3];
    uint32_t hw = h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];
    for (h += 3; *h && hw != nw; hw = hw << 8 | *++h)
        ;
    return *h ? (char *)h - 3 : 0;
}

static char *strstr_twoway(const uint8_t *h, const uint8_t *n)
{
    size_t mem;
    size_t mem0;
    size_t byteset[32 / sizeof(size_t)] = {0};
    size_t shift[256];
    size_t l;

    /* Computing length of needle and fill shift table */
    for (l = 0; n[l] && h[l]; l++)
    {
        BITOP(byteset, n[l], |=);
        shift[n[l]] = l + 1;
    }

    if (n[l])
    {
        return 0; /* hit the end of h */
    }

    /* Compute maximal suffix */
    size_t ip = -1;
    size_t jp = 0;
    size_t k = 1;
    size_t p = 1;
    while (jp + k < l)
    {
        if (n[ip + k] == n[jp + k])
        {
            if (k == p)
            {
                jp += p;
                k = 1;
            }
            else
            {
                k++;
            }
        }
        else if (n[ip + k] > n[jp + k])
        {
            jp += k;
            k = 1;
            p = jp - ip;
        }
        else
        {
            ip = jp++;
            k = p = 1;
        }
    }
    size_t ms = ip;
    size_t p0 = p;

    /* And with the opposite comparison */
    ip = -1;
    jp = 0;
    k = p = 1;
    while (jp + k < l)
    {
        if (n[ip + k] == n[jp + k])
        {
            if (k == p)
            {
                jp += p;
                k = 1;
            }
            else
            {
                k++;
            }
        }
        else if (n[ip + k] < n[jp + k])
        {
            jp += k;
            k = 1;
            p = jp - ip;
        }
        else
        {
            ip = jp++;
            k = p = 1;
        }
    }
    if (ip + 1 > ms + 1)
    {
        ms = ip;
    }
    else
    {
        p = p0;
    }

    /* Periodic needle? */
    if (memcmp(n, n + p, ms + 1))
    {
        mem0 = 0;
        p = MAX(ms, l - ms - 1) + 1;
    }
    else
    {
        mem0 = l - p;
    }
    mem = 0;

    /* Initialize incremental end-of-haystack pointer */
    const uint8_t *z = h;

    /* Search loop */
    for (;;)
    {
        /* Update incremental end-of-haystack pointer */
        if ((size_t)(z - h) < l)
        {
            /* Fast estimate for MIN(l,63) */
            size_t grow = l | 63;
            const uint8_t *z2 = memchr(z, 0, grow);
            if (z2)
            {
                z = z2;
                if ((size_t)(z - h) < l)
                {
                    return 0;
                }
            }
            else
            {
                z += grow;
            }
        }

        /* Check last byte first; advance by shift on mismatch */
        if (BITOP(byteset, h[l - 1], &))
        {
            k = l - shift[h[l - 1]];
            if (k)
            {
                if (mem0 && mem && k < p)
                    k = l - p;
                h += k;
                mem = 0;
                continue;
            }
        }
        else
        {
            h += l;
            mem = 0;
            continue;
        }

        /* Compare right half */
        for (k = MAX(ms + 1, mem); n[k] && n[k] == h[k]; k++)
            ;
        if (n[k])
        {
            h += k - ms;
            mem = 0;
            continue;
        }
        /* Compare left half */
        for (k = ms + 1; k > mem && n[k - 1] == h[k - 1]; k--)
            ;
        if (k <= mem)
        {
            return (char *)h;
        }
        h += p;
        mem = mem0;
    }
}

char *strstr(const char *h, const char *n)
{
    /* Return immediately on empty needle */
    if (!n[0])
    {
        return (char *)h;
    }

    /* Use faster algorithms for short needles */
    h = strchr(h, *n);
    if (!h || !n[1])
    {
        return (char *)h;
    }

    if (!h[1])
        return 0;
    if (!n[2])
        return strstr_2b((void *)h, (void *)n);
    if (!h[2])
        return 0;
    if (!n[3])
        return strstr_3b((void *)h, (void *)n);
    if (!h[3])
        return 0;
    if (!n[4])
        return strstr_4b((void *)h, (void *)n);

    /* Two-way on large needles */
    return strstr_twoway((void *)h, (void *)n);
}

#if 0

long atol(const char *s)
{
    int n = 0;
    int neg = 0;
    while (isspace(*s))
    {
        s++;
    }
    switch (*s)
    {
    case '-':
        neg = 1; /* fallthrough */
    case '+':
        s++;
    }
    while (isdigit(*s))
    {
        n = 10 * n - (*s++ - '0');
    }
    /* The sign order may look incorrect here but this is correct as n is calculated
     * as a negative number to avoid overflow on INT_MAX.
     */
    return neg ? n : -n;
}

int atoi(const char *s)
{
    int n = 0;
    int neg = 0;
    while (isspace(*s))
    {
        s++;
    }
    switch (*s)
    {
    case '-':
        neg = 1; /* fallthrough */
    case '+':
        s++;
    }
    while (isdigit(*s))
    {
        n = 10 * n - (*s++ - '0');
    }
    /* The sign order may look incorrect here but this is correct as n is calculated
     * as a negative number to avoid overflow on INT_MAX.
     */
    return neg ? n : -n;
}
#endif

size_t lfind(const char *str, const char accept)
{
    return (size_t)strchr(str, accept);
}

size_t rfind(const char *str, const char accept)
{
    return (size_t)strrchr(str, accept);
}

char *strtok_r(char *str, const char *delim, char **saveptr)
{
    char *token;
    if (str == NULL)
    {
        str = *saveptr;
    }
    str += strspn(str, delim);
    if (*str == '\0')
    {
        *saveptr = str;
        return NULL;
    }
    token = str;
    str = strpbrk(token, delim);
    if (str == NULL)
    {
        *saveptr = (char *)lfind(token, '\0');
    }
    else
    {
        *str = '\0';
        *saveptr = str + 1;
    }
    return token;
}

char *strtok(char *str, const char *delim)
{
    static char *saveptr = NULL;
    if (str)
    {
        saveptr = NULL;
    }
    return strtok_r(str, delim, &saveptr);
}

char *strcat(char *restrict dest, const char *restrict src)
{
    char *end = dest;
    while (*end != '\0')
    {
        ++end;
    }
    while (*src)
    {
        *end = *src;
        end++;
        src++;
    }
    *end = '\0';
    return dest;
}

char *strncat(char *dest, const char *src, size_t n)
{
    char *end = dest;
    while (*end != '\0')
    {
        ++end;
    }
    size_t i = 0;
    while (*src && i < n)
    {
        *end = *src;
        end++;
        src++;
        i++;
    }
    *end = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    char *out = dest;
    while (n > 0)
    {
        if (!*src)
            break;
        *out = *src;
        ++out;
        ++src;
        --n;
    }
    for (int i = 0; i < (int)n; ++i)
    {
        *out = '\0';
        ++out;
    }
    return out;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;

    while (n-- && *s1 == *s2)
    {
        if (!n || !*s1)
            break;
        s1++;
        s2++;
    }
    return (*(uint8_t *)s1) - (*(uint8_t *)s2);
}

void *memmove(void *dest, const void *src, size_t n)
{
    char *d = dest;
    const char *s = src;

    if (d == s)
    {
        return d;
    }

    if (s + n <= d || d + n <= s)
    {
        return memcpy(d, s, n);
    }

    if (d < s)
    {
        if ((uintptr_t)s % sizeof(size_t) == (uintptr_t)d % sizeof(size_t))
        {
            while ((uintptr_t)d % sizeof(size_t))
            {
                if (!n--)
                {
                    return dest;
                }
                *d++ = *s++;
            }
            for (; n >= sizeof(size_t); n -= sizeof(size_t), d += sizeof(size_t), s += sizeof(size_t))
            {
                *(size_t *)d = *(size_t *)s;
            }
        }
        for (; n; n--)
        {
            *d++ = *s++;
        }
    }
    else
    {
        if ((uintptr_t)s % sizeof(size_t) == (uintptr_t)d % sizeof(size_t))
        {
            while ((uintptr_t)(d + n) % sizeof(size_t))
            {
                if (!n--)
                {
                    return dest;
                }
                d[n] = s[n];
            }
            while (n >= sizeof(size_t))
            {
                n -= sizeof(size_t);
                *(size_t *)(d + n) = *(size_t *)(s + n);
            }
        }
        while (n)
        {
            n--;
            d[n] = s[n];
        }
    }

    return dest;
}

int strcasecmp(const char *s1, const char *s2)
{
    for (; tolower(*s1) == tolower(*s2) && *s1; s1++, s2++)
        ;
    return *(uint8_t *)s1 - *(uint8_t *)s2;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;

    while (n-- && tolower(*s1) == tolower(*s2))
    {
        if (!n || !*s1)
            break;
        s1++;
        s2++;
    }
    return (unsigned int)tolower(*s1) - (unsigned int)tolower(*s2);
}

// Like strncpy but guaranteed to NUL-terminate.
char *safestrncpy(char *restrict s, const char *restrict t, size_t n)
{
    char *os;

    os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}

int compare_strings(const char *s0, const char *s1)
{
    if (strlen(s0) != strlen(s1))
        return -1;
    return strcmp((char *)s0, (char *)s1);
}

#ifdef KALLOC_H
char *strdup(const char *s)
{
    int len = strlen((const char *)s) + 1;
    char *str = (char *)kmalloc(len);
    if (!str)
        return (char *)0;
    strcpy(str, (char *)s);
    str[len - 1] = 0;
    return str;
}

static char *
grabtok(char *s, int *rlen, int c)
{
    int len = 0;
    char *tok = NULL, *buf = NULL;
    if (!s)
        return NULL;
    buf = s;
    while (*s && (*s++ != c))
        len++;
    s = buf;
    tok = kmalloc(len + 1);
    memset(tok, 0, len + 1);
    safestrncpy(tok, s, len + 1);
    if (rlen)
        *rlen = len;
    return tok;
}

char **tokenize(char *s, int c, size_t *ntoks, char **plast_tok) {
    int len = 0;
    size_t i = 0;
    char *buf = NULL;
    char *tmp = NULL, *tmp2 = NULL;
    char **tokens = NULL, *last_tok = NULL;

    if (!s || !c)
        return NULL;

    len = strlen(s);
    buf = kmalloc(len + 1);

    memset(buf, 0, len + 1);
    strncpy(buf, s, len);
    tmp = buf;
    tmp2 = &buf[strlen(buf) - 1];

    while (*tmp2 && (*tmp2 == c))
        *tmp2-- = '\0';

    for (int len = 0; *buf; ++i)
    {
        tokens = krealloc(tokens, (sizeof(char *) * (i + 1)));
        while (*buf && (*buf == c))
            buf++;
        char *tok = grabtok(buf, &len, c);
        if (!tok)
            break;
        last_tok = tokens[i] = tok;
        buf += len;
    }

    tokens = krealloc(tokens, (sizeof(char *) * (i + 1)));
    tokens[i] = NULL;
    kfree(tmp);

    if (ntoks)
        *ntoks = i;
    if (plast_tok)
        *plast_tok = last_tok;
    return tokens;
}

void tokens_free(char **tokens)
{
    if (!tokens)
        return;
    foreach (token, tokens)
        kfree(token);
    kfree(tokens);
}

char **canonicalize_path(const char *path, size_t *ntoks, char **plast)
{
    /* Tokenize slash seperated words in path into tokens */
    char **tokens = tokenize((char *)path, '/', ntoks, plast);
    return tokens;
}

#endif

char *combine_strings(const char *s0, const char *s1)
{
    if (!s0 || !s1)
        return NULL;

    size_t string_len = strlen(s0) + strlen(s1) + 1;
    char *string = (typeof(string))kmalloc(string_len);

    if (!string)
        return NULL;

    memset(string, 0, string_len);
    strcat(string, (char *)s0);
    strcat(string, (char *)s1);
    return string;
}
