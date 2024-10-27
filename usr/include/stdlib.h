#ifndef CORE_STDLIB
#define CORE_STDLIB 1

#define	RAND_MAX	2147483647

extern int rand(void);
extern void srand(unsigned int __seed);


extern void *malloc(unsigned long size);
extern void free(void *ptr);
extern void *calloc(unsigned long nmemb, unsigned long size);
extern void *realloc(void *ptr, unsigned long size);
extern void *reallocarray(void *ptr, unsigned long nmemb, unsigned long size);

#endif // CORE_STDLIB