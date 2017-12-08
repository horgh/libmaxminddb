#include "data-pool.h"
#include "maxminddb.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static bool can_multiply(size_t const, size_t const, size_t const);
static MMDB_entry_data_list_s *data_pool_get_list_head(
    MMDB_entry_data_list_s *const);

// Allocate an MMDB_data_pool_s. It initially has space for size
// MMDB_entry_data_list_s structs.
//
// You must free it using data_pool_destroy().
MMDB_data_pool_s *data_pool_new(size_t const size)
{
    MMDB_data_pool_s *const pool = calloc(1, sizeof(MMDB_data_pool_s));
    if (!pool) {
        return NULL;
    }

    // This should be high enough that we never need to grow the array of
    // pointers to blocks. 32 is more than enough. Even starting out of with
    // size 1 (1 struct), the 32nd element alone will provide us 2**32 structs
    // due to us exponentially increasing the numbers of structs that can fit
    // in each block of memory. Being confident that we do not have to grow the
    // array lets us avoid writing code to do that. That code would be risky as
    // it would likely rarely be hit and so likely not be well tested.
    pool->count = 32;

    if (!can_multiply(SIZE_MAX, pool->count,
                      sizeof(MMDB_entry_data_list_s *))) {
        data_pool_destroy(pool, false);
        return NULL;
    }
    pool->blocks = calloc(pool->count, sizeof(MMDB_entry_data_list_s *));
    if (!pool->blocks) {
        data_pool_destroy(pool, false);
        return NULL;
    }

    if (size == 0 ||
        !can_multiply(SIZE_MAX, size, sizeof(MMDB_entry_data_list_s))) {
        data_pool_destroy(pool, false);
        return NULL;
    }
    pool->size = size;
    pool->blocks[pool->index] = calloc(pool->size,
                                       sizeof(MMDB_entry_data_list_s));
    if (!pool->blocks[pool->index]) {
        data_pool_destroy(pool, false);
        return NULL;
    }

    pool->blocks[pool->index]->head = true;

    return pool;
}

// Determine if we can multiply m*n. We can do this if the result will be below
// the given max. max will typically be SIZE_MAX.
//
// We want to know if we'll wrap around.
static bool can_multiply(size_t const max, size_t const m, size_t const n)
{
    if (m == 0) {
        return false;
    }

    return n <= max / m;
}

// Clean up the data pool.
//
// You can choose to keep the list elements around separate from the pool
// itself by passing keep_list as true. This is useful once you have no further
// use for the pool's management. If you do, you must clean up the list using
// data_pool_list_destroy().
void data_pool_destroy(MMDB_data_pool_s *const pool, bool const keep_list)
{
    if (!pool) {
        return;
    }

    if (pool->blocks) {
        if (!keep_list) {
            for (size_t i = 0; i <= pool->index; i++) {
                free(pool->blocks[i]);
            }
        }

        free(pool->blocks);
    }

    free(pool);
}

// Claim a new struct from the pool. Doing this may cause the pool's size to
// grow.
MMDB_entry_data_list_s *data_pool_alloc(MMDB_data_pool_s *const pool)
{
    if (pool->used < pool->size) {
        MMDB_entry_data_list_s *const element = pool->blocks[pool->index] +
                                                pool->used;

        if (pool->used > 0) {
            MMDB_entry_data_list_s *const prev = pool->blocks[pool->index] +
                                                 pool->used - 1;
            prev->next = element;
        }

        pool->used++;
        return element;
    }

    // Take it from a new block of memory.

    size_t const new_index = pool->index + 1;
    if (new_index == pool->count) {
        // See the comment about not growing this in data_pool_new().
        return NULL;
    }

    if (!can_multiply(SIZE_MAX, pool->size, 2)) {
        return NULL;
    }
    size_t const new_size = pool->size * 2;

    if (!can_multiply(SIZE_MAX, new_size, sizeof(MMDB_entry_data_list_s))) {
        return NULL;
    }
    pool->blocks[new_index] = calloc(new_size, sizeof(MMDB_entry_data_list_s));
    if (!pool->blocks[new_index]) {
        return NULL;
    }

    pool->blocks[new_index]->head = true;
    pool->index = new_index;

    MMDB_entry_data_list_s *const prev = pool->blocks[pool->index - 1]
                                         + pool->size - 1;

    pool->size = new_size;

    MMDB_entry_data_list_s *const element = pool->blocks[pool->index];

    prev->next = element;

    pool->used = 1;
    return element;
}

// Destroy a linked list created using the data pool.
//
// This destroys only the linked list. This is useful if you used
// data_pool_destroy() but kept the list around.
bool data_pool_list_destroy(MMDB_entry_data_list_s *const list)
{
    // There are potentially multiple blocks of memory, each containing
    // multiple list elements. We need to free each block, not each element.

    MMDB_entry_data_list_s *element = list;
    while (element) {
        // We should always be at a head.
        if (!element->head) {
            return false;
        }

        MMDB_entry_data_list_s *const next_element = data_pool_get_list_head(
            element->next);
        free(element);
        element = next_element;
    }

    return true;
}

static MMDB_entry_data_list_s *data_pool_get_list_head(
    MMDB_entry_data_list_s *const list)
{
    MMDB_entry_data_list_s *element = list;
    while (element) {
        if (element->head) {
            return element;
        }

        element = element->next;
    }

    return NULL;
}

#ifdef TEST_DATA_POOL

#include <libtap/tap.h>
#include <maxminddb_test_helper.h>

static void test_can_multiply(void);

int main(void)
{
    plan(NO_PLAN);
    test_can_multiply();
    done_testing();
}

static void test_can_multiply(void)
{
    {
        ok(can_multiply(SIZE_MAX, 1, SIZE_MAX), "1*SIZE_MAX is ok");
    }

    {
        ok(!can_multiply(SIZE_MAX, 2, SIZE_MAX), "2*SIZE_MAX is not ok");
    }

    {
        ok(can_multiply(SIZE_MAX, 10240, sizeof(MMDB_entry_data_list_s)),
           "1024 entry_data_list_s's are okay");
    }
}

#endif
