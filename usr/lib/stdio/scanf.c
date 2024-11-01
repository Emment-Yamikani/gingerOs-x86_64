#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

extern char * _argv_0;

int vsscanf(const char *str, const char *format, va_list ap) {
	int count = 0;
	while (*format) {
		if (*format == ' ') {
			/* handle white space */
			while (*str && isspace(*str)) {
				str++;
			}
		} else if (*format == '%') {
			/* Parse */
			format++;
			__unused int _long = 0;

			if (*format == 'l') {
				format++;
				if (*format == 'l') {
					_long = 1;
					format++;
				}
			}

			if (*format == 'd') {
				int i = 0;
				int sign = 1;
				while (isspace(*str)) str++;
				if (*str == '-') {
					sign = -1;
					str++;
				}
				while (*str && *str >= '0' && *str <= '9') {
					i = i * 10 + *str - '0';
					str++;
				}
				int * out = (int *)va_arg(ap, int*);
				count++;
				*out = i * sign;
			} else if (*format == 'u') {
				unsigned int i = 0;
				while (isspace(*str)) str++;
				while (*str && *str >= '0' && *str <= '9') {
					i = i * 10 + *str - '0';
					str++;
				}
				unsigned int * out = (unsigned int *)va_arg(ap, unsigned int*);
				count++;
				*out = i;
			}
		} else {
			/* Expect exact character? */
			if (*str == *format) {
				str++;
			} else {
				break;
			}
		}
		format++;
	}
	return count;
}

int vfscanf(FILE * stream __unused, const char *format, va_list ap __unused) {
	while (*format) {
		if (*format == ' ') {
			/* Handle whitespace */
		} else if (*format == '%') {
			/* Parse */
		} else {
			/* Expect exact character? */
		}
		format++;
	}
	return 0;
}

int sscanf(const char *str, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int out = vsscanf(str, format, args);
	va_end(args);
	return out;
}

int fscanf(FILE *stream, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int out = vfscanf(stream, format, args);
	va_end(args);
	return out;
}

int scanf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	int out = vfscanf(stdin, format, args);
	va_end(args);
	return out;
}
