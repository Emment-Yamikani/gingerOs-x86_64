#pragma once

#include <lib/types.h>

long sleep(long);

int park(void);
int unpark(tid_t);
void setpark(void);