#pragma once

#include <lib/types.h>

enum __ptrace_request {
	PTRACE_ATTACH,
	PTRACE_CONT,
	PTRACE_DETACH,
	PTRACE_TRACEME,
	PTRACE_GETREGS,
	PTRACE_PEEKDATA,
	PTRACE_SIGNALS_ONLY_PLZ,
	PTRACE_POKEDATA,
	PTRACE_SINGLESTEP
};

enum __ptrace_event {
	PTRACE_EVENT_SYSCALL_ENTER,
	PTRACE_EVENT_SYSCALL_EXIT,
	PTRACE_EVENT_SINGLESTEP,
};

extern long ptrace(enum __ptrace_request request, pid_t pid, void * addr, void * data);
