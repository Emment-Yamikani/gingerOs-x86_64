#pragma once

#include <lib/types.h>

uid_t getuid(void);
gid_t getgid(void);
uid_t geteuid(void);
gid_t getegid(void);

int setuid(uid_t uid);
int setgid(gid_t gid);
int seteuid(uid_t euid);
int setegid(gid_t egid);

int getcwd(char *buf, size_t size);
int chdir(const char *path);