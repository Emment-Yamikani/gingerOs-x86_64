#pragma once

typedef int         pid_t;
typedef int         tid_t;

typedef long        ssize_t;
typedef unsigned    long off_t;
typedef uint16_t    devid_t;

typedef int         uid_t;
typedef int         gid_t;
typedef int         ino_t;
typedef int         mode_t;
typedef long        time_t;
typedef int         susseconds_t;
typedef long        timer_t;
typedef long        clock_t;
typedef long        clockid_t;

typedef void *(*thread_entry_t)(void *);