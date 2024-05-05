#pragma once

#include <lib/types.h>

pid_t fork(void);
void exit(int exit_code);
pid_t getpid(void);
pid_t getppid(void);

pid_t   getsid(pid_t pid);
int     setsid(void);
pid_t   getpgrp(void);
int     setpgrp(void);
int     getpgid(pid_t pid);
pid_t   setpgid(pid_t pid, pid_t pgid);

int execve(const char *pathname, char *const argv[],
           char *const envp[]);