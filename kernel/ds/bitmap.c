#include <bits/errno.h>
#include <ds/bitmap.h>
#include <lib/string.h>
#include <mm/kalloc.h>

#define BITS_PER_USIZE   (sizeof(usize) * 8)

int bitmap_init(bitmap_t *bitmap, usize *bm_array, usize bm_size) {
    if (!bitmap || !bm_array || bm_size == 0) {
        return -EINVAL; // Invalid parameters
    }

    bitmap->bm_map = bm_array;
    bitmap->bm_size = bm_size;
    memset(bitmap->bm_map, 0, sizeof(usize) * bm_size);
    bitmap->bm_lock = SPINLOCK_INIT();

    return 0;
}

int bitmap_alloc(usize bm_size, bitmap_t **ppbm) {
    int     err = 0;
    if (!ppbm || bm_size == 0) {
        return -EINVAL; // Invalid parameters
    }

    // Allocate memory for the bitmap structure
    bitmap_t *bitmap = kmalloc(sizeof(bitmap_t));
    if (!bitmap) {
        return -ENOMEM; // Allocation failed
    }

    // Allocate memory for the bitmap array
    usize *bm_array = kmalloc(sizeof(usize) * bm_size);
    if (!bm_array) {
        return -ENOMEM; // Allocation failed
    }

    if ((err = bitmap_init(bitmap, bm_array, bm_size) < 0)) {
        return -err; // Initialization failed
    }

    *ppbm = bitmap;
    return 0;
}

void bitmap_free(bitmap_t *bitmap) {
    if (!bitmap) {
        return; // Invalid bitmap
    }

    bitmap_assert(bitmap);
    bitmap->bm_size = 0;
    
    if (bitmap->bm_map)
        kfree(bitmap->bm_map);
    
    bitmap->bm_map = NULL;
    kfree(bitmap);
}

// Set a range of bits in the bitmap, with checking to prevent setting already set bits
int bitmap_set(bitmap_t *bitmap, usize pos, usize nbits) {
    bitmap_lock(bitmap);
    if (pos + nbits > bitmap->bm_size) {
        bitmap_unlock(bitmap);
        return -EINVAL; // Out of range
    }

    // Check if any bit in the range is already set
    for (usize i = 0; i < nbits; ++i) {
        usize index = (pos + i) / BITS_PER_USIZE;
        usize bit = (pos + i) % BITS_PER_USIZE;
        if (bitmap->bm_map[index] & (1UL << bit)) {
            bitmap_unlock(bitmap);
            return -EALREADY; // Attempt to set an already set bit
        }
    }

    // Set the bits in the range
    for (usize i = 0; i < nbits; ++i) {
        usize index = (pos + i) / BITS_PER_USIZE;
        usize bit = (pos + i) % BITS_PER_USIZE;
        bitmap->bm_map[index] |= (1UL << bit);
    }

    bitmap_unlock(bitmap);

    return 0; // Success
}

// Unset a range of bits in the bitmap, with checking to prevent unsetting already unset bits
int bitmap_unset(bitmap_t *bitmap, usize pos, usize nbits) {
    bitmap_lock(bitmap);
    if (pos + nbits > bitmap->bm_size) {
        bitmap_unlock(bitmap);
        return -EINVAL; // Out of range
    }

    // Check if any bit in the range is already unset
    for (usize i = 0; i < nbits; ++i) {
        usize index = (pos + i) / BITS_PER_USIZE;
        usize bit = (pos + i) % BITS_PER_USIZE;
        if ((bitmap->bm_map[index] & (1UL << bit)) == 0) {
            bitmap_unlock(bitmap);
            return -EALREADY; // Attempt to unset an already unset bit
        }
    }

    // Unset the bits in the range
    for (usize i = 0; i < nbits; ++i) {
        usize index = (pos + i) / BITS_PER_USIZE;
        usize bit = (pos + i) % BITS_PER_USIZE;
        bitmap->bm_map[index] &= ~(1UL << bit);
    }

    bitmap_unlock(bitmap);

    return 0; // Success
}

// Test if a range of bits is all set
int bitmap_test(bitmap_t *bitmap, usize pos, usize nbits) {
    bitmap_lock(bitmap);
    if (pos + nbits > bitmap->bm_size) {
        bitmap_unlock(bitmap);
        return -EINVAL; // Out of range
    }

    // Check if all bits in the range are set
    for (usize i = 0; i < nbits; ++i) {
        usize index = (pos + i) / BITS_PER_USIZE;
        usize bit = (pos + i) % BITS_PER_USIZE;
        if ((bitmap->bm_map[index] & (1UL << bit)) == 0) {
            bitmap_unlock(bitmap);
            return 0; // At least one bit is unset
        }
    }

    bitmap_unlock(bitmap);
    return 1; // All bits are set
}

// Allocate a contiguous range of nbits in the bitmap
int bitmap_alloc_range(bitmap_t *bitmap, usize nbits, usize *pos) {
    bitmap_lock(bitmap);

    if (nbits == 0 || (nbits > bitmap->bm_size) || !pos) {
        bitmap_unlock(bitmap);
        return -EINVAL; // Invalid input
    }

    // Iterate through the bitmap to find a free range of nbits
    for (usize i = 0; i <= bitmap->bm_size - nbits; ++i) {
        int range_free = 1;

        // Check if the range starting at `i` is free
        for (usize j = 0; j < nbits; ++j) {
            usize index = (i + j) / BITS_PER_USIZE;
            usize bit = (i + j) % BITS_PER_USIZE;
            if (bitmap->bm_map[index] & (1UL << bit)) {
                range_free = 0;
                break;
            }
        }

        // If the range is free, allocate it
        if (range_free) {
            for (usize j = 0; j < nbits; ++j) {
                usize index = (i + j) / BITS_PER_USIZE;
                usize bit = (i + j) % BITS_PER_USIZE;
                bitmap->bm_map[index] |= (1UL << bit);
            }
            *pos = i;
            bitmap_unlock(bitmap);
            return 0; // Success
        }
    }

    bitmap_unlock(bitmap);
    return -ENOMEM; // No free range found
}

/**
 * Dumps the entire bitmap in hexadecimal format.
 * Dump the entire bitmap in a formatted table with proper column alignment
 * 
 * @param bitmap The bitmap to dump.
 */
void bitmap_dump(bitmap_t *bitmap) {
    bitmap_lock(bitmap);

    usize num_units = (bitmap->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE; // Number of usize units
    
    printk("\nDumping Bitmap Info: Size (bits): %lu, Map Ptr: %p\n",
        bitmap->bm_size, (void *)bitmap->bm_map
    );

    // Iterate through all the units in the bitmap
    for (usize i = 0; i < num_units; i += 4) {
        usize first_index = i;
        usize last_index = first_index + 3;

        if (last_index >= num_units) {
            last_index = num_units - 1;
        }

        // Print row header
        printk("Index<%5lu - %5lu> ", first_index, last_index);

        // Iterate through the columns in this row
        for (usize j = 0; j < 4; ++j) {
            usize current_index = first_index + j;

            if (current_index >= num_units) {
                break; // Skip columns outside the range
            }

            usize bit_start = current_index * BITS_PER_USIZE;
            usize bit_end = bit_start + BITS_PER_USIZE - 1;

            // Print the hex value and bit range with aligned columns
            printk("|%016lx|[%6lu - %6lu] ", bitmap->bm_map[current_index], bit_start, bit_end);
        }

        printk("|\n");
    }

    printk("\n");
    bitmap_unlock(bitmap);
}

/**
 * Dumps a specific range of the bitmap in hexadecimal format.
 * Dump a specific range of the bitmap in hexadecimal format as a table of 4 columns
 * Dump a specific range of the bitmap in hexadecimal format as a table of 4 columns
 * Each row shows the integer index and the range of bits it represents.
 * Dump a specific range of the bitmap in a formatted table with proper column alignment
 * 
 * @param bitmap The bitmap to dump.
 * @param start  The starting bit position.
 * @param end    The ending bit position.
 */
void bitmap_dump_range(bitmap_t *bitmap, usize start, usize end) {
    bitmap_lock(bitmap);

    if (start >= end || end > bitmap->bm_size) {
        bitmap_unlock(bitmap);
        printk("Invalid range\n");
        return;
    }

    usize start_index = start / BITS_PER_USIZE;
    usize end_index = (end + BITS_PER_USIZE - 1) / BITS_PER_USIZE; // Round up to include the end bit
    usize total_units = end_index - start_index;

    
    printk("\nDumping Bitmap Info: Size (bits): %lu, Map Ptr: %p\n",
        bitmap->bm_size, (void *)bitmap->bm_map
    );

    // Iterate through the relevant part of the bitmap
    for (usize i = 0; i < total_units; i += 4) {
        usize first_index = start_index + i;
        usize last_index = first_index + 3;

        if (last_index >= end_index) {
            last_index = end_index - 1;
        }

        // Print row header
        printk("Index<%5lu - %5lu> ", first_index, last_index);

        // Iterate through the columns in this row
        for (usize j = 0; j < 4; ++j) {
            usize current_index = first_index + j;

            if (current_index >= end_index) {
                break; // Skip columns outside the range
            }

            usize bit_start = current_index * BITS_PER_USIZE;
            usize bit_end = bit_start + BITS_PER_USIZE - 1;

            // Ensure bit ranges are clamped to the requested range
            if (bit_start < start) {
                bit_start = start;
            }
            if (bit_end >= end) {
                bit_end = end - 1;
            }

            // Print the hex value and bit range with aligned columns
            printk("|%016lx|[%6lu - %6lu] ", bitmap->bm_map[current_index], bit_start, bit_end);
        }

        printk("|\n");
    }

    printk("\n");
    bitmap_unlock(bitmap);
}

void bitmap_dump_with_columns(bitmap_t *bitmap, usize num_columns) {
    if (num_columns == 0) {
        printk("Error: Number of columns must be greater than 0.\n");
        return;
    }

    bitmap_lock(bitmap);

    usize num_units = (bitmap->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE; // Number of usize units
    
    printk("\nDumping Bitmap Info: Size (bits): %lu, Map Ptr: %p\n",
        bitmap->bm_size, (void *)bitmap->bm_map
    );

    for (usize i = 0; i < num_units; i += num_columns) {
        usize first_index = i;
        usize last_index = first_index + num_columns - 1;

        if (last_index >= num_units) {
            last_index = num_units - 1;
        }

        // Print row header
        printk("Index<%5lu - %5lu> ", first_index, last_index);

        for (usize j = 0; j < num_columns; ++j) {
            usize current_index = first_index + j;

            if (current_index >= num_units) {
                break; // Skip columns outside the range
            }

            usize bit_start = current_index * BITS_PER_USIZE;
            usize bit_end = bit_start + BITS_PER_USIZE - 1;

            // Print the hex value and bit range with aligned columns
            printk("|%016lx|[%6lu-%6lu] ", bitmap->bm_map[current_index], bit_start, bit_end);
        }

        printk("|\n");
    }

    printk("\n");
    bitmap_unlock(bitmap);
}

void bitmap_dump_range_with_columns(bitmap_t *bitmap, usize start, usize end, usize num_columns) {
    if (num_columns == 0) {
        printk("Error: Number of columns must be greater than 0.\n");
        return;
    }

    bitmap_lock(bitmap);

    if (start >= bitmap->bm_size || end >= bitmap->bm_size || start > end) {
        printk("Error: Invalid range specified.\n");
        bitmap_unlock(bitmap);
        return;
    }

    usize start_unit = start / BITS_PER_USIZE;
    usize end_unit = end / BITS_PER_USIZE;
    // usize num_units = end_unit - start_unit + 1;

    
    printk("\nDumping Bitmap Info: Size (bits): %lu, Map Ptr: %p\n",
        bitmap->bm_size, (void *)bitmap->bm_map
    );

    for (usize i = start_unit; i <= end_unit; i += num_columns) {
        usize first_index = i;
        usize last_index = first_index + num_columns - 1;

        if (last_index > end_unit) {
            last_index = end_unit;
        }

        // Print row header
        printk("Index<%5lu - %5lu> ", first_index, last_index);

        for (usize j = 0; j < num_columns; ++j) {
            usize current_index = first_index + j;

            if (current_index > end_unit) {
                break; // Skip columns outside the range
            }

            usize bit_start = current_index * BITS_PER_USIZE;
            usize bit_end = bit_start + BITS_PER_USIZE - 1;

            // Clip the bit range to the specified range
            if (bit_start < start) {
                bit_start = start;
            }
            if (bit_end > end) {
                bit_end = end;
            }

            // Print the hex value and bit range with aligned columns
            printk("|%016lx|[%6lu-%6lu] ", bitmap->bm_map[current_index], bit_start, bit_end);
        }

        printk("|\n");
    }

    printk("\n");
    bitmap_unlock(bitmap);
}

void bitmap_clear_all(bitmap_t *bitmap) {
    bitmap_lock(bitmap);
    usize num_units = (bitmap->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE;
    for (usize i = 0; i < num_units; i++) {
        bitmap->bm_map[i] = 0;
    }
    bitmap_unlock(bitmap);
}

void bitmap_set_all(bitmap_t *bitmap) {
    bitmap_lock(bitmap);
    usize num_units = (bitmap->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE;
    for (usize i = 0; i < num_units; i++) {
        bitmap->bm_map[i] = ~(usize)0;
    }

    // Clear any extra bits outside the valid range
    usize excess_bits = num_units * BITS_PER_USIZE - bitmap->bm_size;
    if (excess_bits > 0) {
        bitmap->bm_map[num_units - 1] &= (~((usize)0)) >> excess_bits;
    }

    bitmap_unlock(bitmap);
}

int bitmap_find_first_unset(bitmap_t *bitmap, usize *pos) {
    bitmap_lock(bitmap);
    usize num_units = (bitmap->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE;

    for (usize i = 0; i < num_units; i++) {
        if (bitmap->bm_map[i] != ~(usize)0) { // Not fully set
            for (usize bit = 0; bit < BITS_PER_USIZE; bit++) {
                usize bit_pos = i * BITS_PER_USIZE + bit;
                if (bit_pos >= bitmap->bm_size) {
                    break; // Out of bounds
                }
                if (!(bitmap->bm_map[i] & (1UL << bit))) {
                    *pos = bit_pos;
                    bitmap_unlock(bitmap);
                    return 0; // Found unset bit
                }
            }
        }
    }

    bitmap_unlock(bitmap);
    return -ENOENT; // No unset bit found
}

usize bitmap_count_set(bitmap_t *bitmap) {
    bitmap_lock(bitmap);
    usize count = 0;
    usize num_units = (bitmap->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE;

    for (usize i = 0; i < num_units; i++) {
        usize val = bitmap->bm_map[i];
        while (val) {
            count += val & 1;
            val >>= 1;
        }
    }

    bitmap_unlock(bitmap);
    return count;
}

usize bitmap_count_unset(bitmap_t *bitmap) {
    return bitmap->bm_size - bitmap_count_set(bitmap);
}

int bitmap_resize(bitmap_t *bitmap, usize new_size) {
    bitmap_lock(bitmap);

    usize old_num_units = (bitmap->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE;
    usize new_num_units = (new_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE;

    // Allocate new memory if required
    usize *new_map = krealloc(bitmap->bm_map, new_num_units * sizeof(usize));
    if (!new_map) {
        bitmap_unlock(bitmap);
        return -ENOMEM; // Memory allocation failed
    }

    bitmap->bm_map = new_map;

    // Clear any new units
    if (new_num_units > old_num_units) {
        for (usize i = old_num_units; i < new_num_units; i++) {
            bitmap->bm_map[i] = 0;
        }
    }

    // Adjust for new size
    bitmap->bm_size = new_size;

    // Clear excess bits in the last unit
    usize excess_bits = new_num_units * BITS_PER_USIZE - new_size;
    if (excess_bits > 0) {
        bitmap->bm_map[new_num_units - 1] &= (~((usize)0)) >> excess_bits;
    }

    bitmap_unlock(bitmap);
    return 0;
}

int bitmap_toggle(bitmap_t *bitmap, usize pos, usize nbits) {
    if (pos + nbits > bitmap->bm_size) {
        return -EINVAL; // Out of bounds
    }

    bitmap_lock(bitmap);

    for (usize i = 0; i < nbits; i++) {
        usize bit_pos = pos + i;
        usize unit_index = bit_pos / BITS_PER_USIZE;
        usize bit_index = bit_pos % BITS_PER_USIZE;
        bitmap->bm_map[unit_index] ^= (1UL << bit_index);
    }

    bitmap_unlock(bitmap);
    return 0;
}

int bitmap_copy(bitmap_t *src, bitmap_t *dest) {
    bitmap_lock(src);

    dest->bm_size = src->bm_size;
    usize num_units = (src->bm_size + BITS_PER_USIZE - 1) / BITS_PER_USIZE;
    dest->bm_map = kmalloc(num_units * sizeof(usize));
    if (!dest->bm_map) {
        bitmap_unlock(src);
        return -EINVAL; // Memory allocation failed
    }

    memcpy(dest->bm_map, src->bm_map, num_units * sizeof(usize));
    dest->bm_lock = SPINLOCK_INIT();

    bitmap_unlock(src);
    return 0;
}
