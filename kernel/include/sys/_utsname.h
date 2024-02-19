#pragma once

#define _UTSNAME_LENGTH 65

typedef struct utsname {
char  sysname[_UTSNAME_LENGTH];    // Name of this implementation of the operating system. 
char  nodename[_UTSNAME_LENGTH];   // Name of this node within the communications 
                    // network to which this node is attached, if any. 
char  release[_UTSNAME_LENGTH];    // Current release level of this implementation. 
char  version[_UTSNAME_LENGTH];    // Current version level of this release. 
char  machine[_UTSNAME_LENGTH];    // Name of the hardware type on which the system is running. 
} utsname_t;
// The character arrays are of unspecified size, but the data stored in them shall be terminated by a null byte.

int uname(utsname_t *);