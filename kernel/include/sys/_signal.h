#pragma once

#include <lib/stdint.h>

typedef struct {
    uint64_t ignore;    // signals to ignore.
    uint64_t pending;   // signals to acknowledge.
} signal_t;

#define SIGABRT     1;
#define SIGALRM     2;
#define SIGBUS      3;
#define SIGCANCEL   4;
#define SIGCHLD     5;
#define SIGCONT     6;
#define SIGEMT      7;
#define SIG      8;