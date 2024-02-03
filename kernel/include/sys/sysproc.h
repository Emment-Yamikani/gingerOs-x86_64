#pragma once

#include <lib/types.h>

pid_t fork(void);
void exit(int exit_code);
pid_t getpid(void);
pid_t getppid(void);