#include <sys/schedule.h>

#define sched_level_lock(lvl)      ({ spin_lock(&(lvl)->lock); })
#define sched_level_unlock(lvl)    ({ spin_lock(&(lvl)->lock); })