#ifndef DATA_POOL_H
#define DATA_POOL_H

#include "maxminddb.h"

#include <stdbool.h>
#include <stddef.h>

// A pool of memory for MMDB_entry_data_list_s structs. This is so we can
// allocate multiple up front rather than one at a time for performance
// reasons.
//
// You can think of the pool as an array. The order you add elements to it (by
// calling data_pool_alloc()) ends up as the order of the list.
//
// The memory only grows. There is no support for releasing an element you take
// back to the pool.
typedef struct MMDB_data_pool_s {
    MMDB_entry_data_list_s **blocks;

    // How many blocks there are.
    size_t count;

    // Which block we're allocating out of.
    size_t index;

    // The size of the current block, counting by structs.
    size_t size;

    // How many used in the current block, counting by structs.
    size_t used;
} MMDB_data_pool_s;

MMDB_data_pool_s *data_pool_new(size_t const);
void data_pool_destroy(MMDB_data_pool_s *const, bool const);
MMDB_entry_data_list_s *data_pool_alloc(MMDB_data_pool_s *const);
bool data_pool_list_destroy(MMDB_entry_data_list_s *const);

#endif
