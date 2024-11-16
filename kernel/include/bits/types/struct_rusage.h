#pragma once

#ifndef __rusage_defined
#define __rusage_defined 1

#include <bits/types/struct_timeval.h>

/* Structure which says how much of each resource has been used.  If
   the system does not keep track of a particular value, the struct
   field is always zero.  */

/* The purpose of all the unions is to have the kernel-compatible layout
   while keeping the API type as 'long int', and among machines where
   __syscall_slong_t is not 'long int', this only does the right thing
   for little-endian ones, like x32.  */
struct rusage
{
/* Total amount of user time used.  */
struct timeval ru_utime;
/* Total amount of system time used.  */
struct timeval ru_stime;
/* Maximum resident set size (in kilobytes).  */
__extension__ union
    {
long int ru_maxrss;
__syscall_slong_t __ru_maxrss_word;
    };
/* Amount of sharing of text segment memory
    with other processes (kilobyte-seconds).  */
__extension__ union
    {
long int ru_ixrss;
__syscall_slong_t __ru_ixrss_word;
    };
/* Amount of data segment memory used (kilobyte-seconds).  */
__extension__ union
    {
long int ru_idrss;
__syscall_slong_t __ru_idrss_word;
    };
/* Amount of stack memory used (kilobyte-seconds).  */
__extension__ union
    {
long int ru_isrss;
    __syscall_slong_t __ru_isrss_word;
    };
/* Number of soft page faults (i.e. those serviced by reclaiming
    a page from the list of pages awaiting reallocation.  */
__extension__ union
    {
long int ru_minflt;
__syscall_slong_t __ru_minflt_word;
    };
/* Number of hard page faults (i.e. those that required I/O).  */
__extension__ union
    {
long int ru_majflt;
__syscall_slong_t __ru_majflt_word;
    };
/* Number of times a process was swapped out of physical memory.  */
__extension__ union
    {
long int ru_nswap;
__syscall_slong_t __ru_nswap_word;
    };
/* Number of input operations via the file system.  Note: This
    and `ru_oublock' do not include operations with the cache.  */
__extension__ union
    {
long int ru_inblock;
__syscall_slong_t __ru_inblock_word;
    };
/* Number of output operations via the file system.  */
__extension__ union
    {
long int ru_oublock;
__syscall_slong_t __ru_oublock_word;
    };
/* Number of IPC messages sent.  */
__extension__ union
    {
long int ru_msgsnd;
__syscall_slong_t __ru_msgsnd_word;
    };
/* Number of IPC messages received.  */
__extension__ union
    {
long int ru_msgrcv;
__syscall_slong_t __ru_msgrcv_word;
    };
/* Number of signals delivered.  */
__extension__ union
    {
long int ru_nsignals;
__syscall_slong_t __ru_nsignals_word;
    };
/* Number of voluntary context switches, i.e. because the process
    gave up the process before it had to (usually to wait for some
    resource to be available).  */
__extension__ union
    {
long int ru_nvcsw;
__syscall_slong_t __ru_nvcsw_word;
    };
/* Number of involuntary context switches, i.e. a higher priority process
    became runnable or the current process used up its time slice.  */
__extension__ union
    {
long int ru_nivcsw;
__syscall_slong_t __ru_nivcsw_word;
    };
};