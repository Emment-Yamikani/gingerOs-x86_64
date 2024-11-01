#pragma once

#include <bits/errno.h>
#include <bits/dirent.h>
#include <bits/fcntl.h>
#include <bits/waitflags.h>
#include <bits/waitstatus.h>

#include <ginger/atomic.h>
#include <ginger/spinlock.h>
#include <ginger/syscall.h>
#include <ginger/rtc.h>

#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/system.h>

#include <cpuid.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
// #include <nanojpeg.c>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <types.h>

dev_t mkdev(int major, int minor);