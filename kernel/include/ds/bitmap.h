#pragma once

#include <core/spinlock.h>

typedef unsigned long usize;

typedef struct bitmap_t {
    usize       *bm_map;
    usize       bm_size;
    spinlock_t  bm_lock;
} bitmap_t;

#define bitmap_assert(bm)           ({ assert(bm, "Invalid bitmap!"); })
#define bitmap_lock(bm)             ({bitmap_assert(bm); spin_lock(&(bm)->bm_lock); })
#define bitmap_unlock(bm)           ({bitmap_assert(bm); spin_unlock(&(bm)->bm_lock); })
#define bitmap_islocked(bm)         ({bitmap_assert(bm); spin_islocked(&(bm)->bm_lock); })
#define bitmap_assert_locked(bm)    ({bitmap_assert(bm); spin_assert_locked(&(bm)->bm_lock); })

extern int bitmap_init(bitmap_t *bitmap, usize *bm_array, usize bm_size);

extern int bitmap_alloc(usize bm_size, bitmap_t **ppbm);
extern void bitmap_free(bitmap_t *bitmap);

extern int bitmap_set(bitmap_t *bitmap, usize pos, usize nbits);
extern int bitmap_unset(bitmap_t *bitmap, usize pos, usize nbits);
extern int bitmap_test(bitmap_t *bitmap, usize pos, usize nbits);
extern int bitmap_alloc_range(bitmap_t *bitmap, usize nbits, usize *pos);

/**
 * Dumps the entire bitmap in hexadecimal format.
 *
 * @param bitmap
*/
extern void bitmap_dump(bitmap_t *bitmap);

/**
 * Dumps a specific range of the bitmap in hexadecimal format.
 *
 * @param bitmap The bitmap to dump.
 * @param start  The starting bit position.
 * @param end    The ending bit position.
 */
extern void bitmap_dump_range(bitmap_t *bitmap, usize start, usize end);

void bitmap_dump_with_columns(bitmap_t *bitmap, usize num_columns);
void bitmap_dump_range_with_columns(bitmap_t *bitmap, usize start, usize end, usize num_columns);

void bitmap_clear_all(bitmap_t *bitmap);
void bitmap_set_all(bitmap_t *bitmap);
int bitmap_find_first_unset(bitmap_t *bitmap, usize *pos);
usize bitmap_count_set(bitmap_t *bitmap);
usize bitmap_count_unset(bitmap_t *bitmap);
int bitmap_toggle(bitmap_t *bitmap, usize pos, usize nbits);
int bitmap_copy(bitmap_t *src, bitmap_t *dest);