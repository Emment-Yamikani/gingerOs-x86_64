///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2019, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and snprintf implementation, optimized for speed on
//        embedded systems with a very limited resources.
//        Use this instead of bloated standard/newlib printf.
//        These routines are thread safe and reentrant.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PRINTF_H_
#define _PRINTF_H_

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Output a character to a custom device like UART, used by the printf() function
 * This function is declared here only. You have to write your custom implementation somewhere
 * \param character Character to output
 */
void _putchar(char character);


/**
 * Tiny printf implementation
 * You have to implement _putchar if you use printf()
 * To avoid conflicts with the regular printf() API it is overridden by macro defines
 * and internal underscore-appended functions like printf() are used
 * \param format A string that specifies the format of the output
 * \return The number of characters that are written into the array, not counting the terminating null character
 */
extern int printf(const char* format, ...);


/**
 * Tiny sprintf implementation
 * Due to security reasons (buffer overflow) YOU SHOULD CONSIDER USING (V)SNPRINTF INSTEAD!
 * \param buffer A pointer to the buffer where to store the formatted string. MUST be big enough to store the output!
 * \param format A string that specifies the format of the output
 * \return The number of characters that are WRITTEN into the buffer, not counting the terminating null character
 */
int sprintf(char* buffer, const char* format, ...);


/**
 * Tiny snprintf/vsnprintf implementation
 * \param buffer A pointer to the buffer where to store the formatted string
 * \param count The maximum number of characters to store in the buffer, including a terminating null character
 * \param format A string that specifies the format of the output
 * \param va A value identifying a variable arguments list
 * \return The number of characters that COULD have been written into the buffer, not counting the terminating
 *         null character. A value equal or larger than count indicates truncation. Only when the returned value
 *         is non-negative and less than count, the string has been completely written.
 */
int  snprintf(char* buffer, size_t count, const char* format, ...);
int vsnprintf(char* buffer, size_t count, const char* format, va_list va);


/**
 * Tiny vprintf implementation
 * \param format A string that specifies the format of the output
 * \param va A value identifying a variable arguments list
 * \return The number of characters that are WRITTEN into the buffer, not counting the terminating null character
 */
int vprintf(const char* format, va_list va);


/**
 * printf with output function
 * You may use this as dynamic alternative to printf() with its fixed _putchar() output
 * \param out An output function which takes one character and an argument pointer
 * \param arg An argument pointer for user data passed to output function
 * \param format A string that specifies the format of the output
 * \return The number of characters that are sent to the output function, not counting the terminating null character
 */
int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...);


#ifdef __cplusplus
}
#endif

#define SEEK_SET 0 /* SEEK_SET */
#define SEEK_CUR 1 /* SEEK_CUR */
#define SEEK_END 2 /* SEEK_END */

#include "sys/system.h"

void panic(const char *restrict __fmt__, ...);

#define assert_msg(condition, ...) ({ \
    if ((condition) == 0)             \
        panic(__VA_ARGS__);           \
})

#define assert(condition, msg) ({                      \
    assert_msg(condition, "%s:%d: retaddr: %p: %s\n",  \
               __FILE__, __LINE__, __retaddr(0), msg); \
})

typedef struct _FILE FILE;
#define __DEFINED_FILE

#define BUFSIZ 8192

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern FILE *fopen(const char *path, const char *mode);
extern int fclose(FILE *stream);
extern int fseek(FILE *stream, long offset, int whence);
extern long ftell(FILE *stream);
extern FILE *fdopen(int fd, const char *mode);
extern FILE *freopen(const char *path, const char *mode, FILE *stream);

extern size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
extern size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

extern int fileno(FILE *stream);
extern int fflush(FILE *stream);

extern int vasprintf(char **buf, const char *fmt, va_list args);
extern int sprintf(char *buf, const char *fmt, ...);
extern int fprintf(FILE *stream, const char *fmt, ...);
extern int printf(const char *fmt, ...);
extern int snprintf(char *buf, size_t size, const char *fmt, ...);
extern int vsprintf(char *buf, const char *fmt, va_list args);
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
extern int vfprintf(FILE *device, const char *format, va_list ap);
extern int vprintf(const char *format, va_list ap);

extern int puts(const char *s);
extern int fputs(const char *s, FILE *stream);
extern int fputc(int c, FILE *stream);
extern int putc(int c, FILE *stream);
extern int putchar(int c);
extern int fgetc(FILE *stream);
extern int getc(FILE *stream);
extern char *fgets(char *s, int size, FILE *stream);
extern int getchar(void);

extern void rewind(FILE *stream);
extern void setbuf(FILE *stream, char *buf);

extern void perror(const char *s);

extern int ungetc(int c, FILE *stream);

extern int feof(FILE *stream);
extern void clearerr(FILE *stream);
extern int ferror(FILE *stream);

extern char *strerror(int errnum);

extern int _fwouldblock(FILE *stream);

extern FILE *tmpfile(void);

extern int setvbuf(FILE *stream, char *buf, int mode, size_t size);

extern int remove(const char *pathname);
extern int rename(const char *oldpath, const char *newpath);

#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2

extern char *tmpnam(char *s);
#define L_tmpnam 256

extern int vsscanf(const char *str, const char *format, va_list ap);
extern int sscanf(const char *str, const char *format, ...);
extern int vfscanf(FILE *stream, const char *format, va_list ap);
extern int fscanf(FILE *stream, const char *format, ...);
extern int scanf(const char *format, ...);

typedef long fpos_t;

extern int fgetpos(FILE *stream, fpos_t *pos);
extern int fsetpos(FILE *stream, const fpos_t *pos);

#endif  // _PRINTF_H_
