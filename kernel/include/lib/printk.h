#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>

void panic(const char *restrict __fmt__, ...);
size_t printk(const char *restrict __fmt__, ...);
size_t klog(int type, const char *restrict __fmt__, ...);
#define  assert_msg(condition, ...) { if ((condition)==0) {panic(__VA_ARGS__);} }
#define assert(condition, msg) ({ assert_msg(condition, "%s:%d: retaddr: %p: %s\n", __FILE__, __LINE__, __retaddr(0), msg); })