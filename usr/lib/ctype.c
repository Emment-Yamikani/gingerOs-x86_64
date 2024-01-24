#include "../include/api.h"

int isalpha(int c) {
    return (islower(c) || isupper(c));
}

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isupper(int c) {
    return (c >= 'A' && c <= 'Z');
}

int islower(int c) {
    return (c >= 'a' && c <= 'z');
}

int tolower(int c) {
    if (!isalpha(c))
        return c;
    return OR(c, BS(5));
}

int toupper(int c) {
    if (!isalpha(c))
        return c;
    return NAND(BS(5), c);
}

int isspace(int c) {
    return (c == ' ' || c == '\v' || c == '\b' || c == '\f'
        || c == '\t' || c == '\r' || c == '\n');
}

int isxdigit(int c) {
    return (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

int isalphanum(int c) {
    return isalpha(c) || isdigit(c);
}

long atol(const char * s) {
	int n = 0;
	int neg = 0;
	while (isspace(*s)) {
		s++;
	}
	switch (*s) {
		case '-':
			neg = 1; /* fallthrough */
		case '+':
			s++;
	}
	while (isdigit(*s)) {
		n = 10*n - (*s++ - '0');
	}
	/* The sign order may look incorrect here but this is correct as n is calculated
	 * as a negative number to avoid overflow on INT_MAX.
	 */
	return neg ? n : -n;
}

int atoi(const char * s) {
	int n = 0;
	int neg = 0;
	while (isspace(*s)) {
		s++;
	}
	switch (*s) {
		case '-':
			neg = 1; /* fallthrough */
		case '+':
			s++;
	}
	while (isdigit(*s)) {
		n = 10*n - (*s++ - '0');
	}
	/* The sign order may look incorrect here but this is correct as n is calculated
	 * as a negative number to avoid overflow on INT_MAX.
	 */
	return neg ? n : -n;
}