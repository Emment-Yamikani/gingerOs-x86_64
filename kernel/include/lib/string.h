#pragma once

#include <lib/stddef.h>
#include <lib/stdint.h>

void    *memchr(const void *s, int c, size_t n);
void    *memrchr(const void *m, int c, size_t n);
int      memcmp(const void *s1, const void *s2, size_t n);
void    *memcpy(void *restrict s1, const void *restrict s2, size_t n);
void    *memmove(void *dest, const void *src, size_t n);
void    *memset(void *s, int c, size_t n);
void    *memsetw(void *s, int c, size_t n);

char    *safestrncpy(char *restrict dest, const char *restrict src, size_t n);

long    atol(const char *s);
int     atoi(const char *s);

size_t  lfind(const char *str, const char accept);
size_t  rfind(const char *str, const char accept);
char    *strtok_r(char *str, const char *delim, char **saveptr);
char    *strtok(char *str, const char *delim);

char    *stpcpy(char *restrict d, const char *restrict s);
size_t  strspn(const char *s, const char *c);
char    *strchrnul(const char *s, int c);
char    *strcat(char *restrict dest, const char *restrict src);
char    *strchr(const char *s, int c);
char    *strrchr(const char *s, int c);
size_t  strcspn(const char *s, const char *c);
char    *strpbrk(const char *s, const char *b);
int      strcmp(const char *s1, const char *s2);
char    *strcpy(char *restrict dest, const char *restrict src);
char    *strdup(const char *s);
size_t   strlen(const char *s);

char    *strncat(char *restrict dest, const char *restrict src, size_t n);
int      strncmp(const char *s1, const char *s2, size_t n);
char    *strncpy(char *restrict dest, const char *restrict src, size_t n);
char    *strndup(const char *s, size_t n);
char    *strrchr(const char *s, int c);

int     strcasecmp(const char *s1, const char *s2);
int     strncasecmp(const char *s1, const char *s2, size_t n);

/// test strings for equality, based of contents.
/// @returns 1 on success and 0 no failure. 
int     string_eq(const char *str0, const char *str1);
int     compare_strings(const char *s0, const char *s1);
char    *combine_strings(const char *s0, const char *s1);
void    tokens_free(char **tokens);
int     tokenize(char *s, int c, size_t *ntoks, char ***tokenized, char **plast_tok);
int     canonicalize_path(const char *path, size_t *ntoks, char ***ptokenized, char **plast);