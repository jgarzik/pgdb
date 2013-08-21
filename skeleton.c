/* Copyright (c) 2011 The LevelDB Authors. All rights reserved.
  Use of this source code is governed by a BSD-style license that can be
  found in the LICENSE file. See the AUTHORS file for names of contributors.

  Copyright 2013 BitPay, Inc.
*/

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "pgdb.h"

/* DB operations */

void pgdb_put(
    pgdb_t* db,
    const pgdb_writeoptions_t* options,
    const char* key, size_t keylen,
    const char* val, size_t vallen,
    char** errptr)
{
}

void pgdb_delete(
    pgdb_t* db,
    const pgdb_writeoptions_t* options,
    const char* key, size_t keylen,
    char** errptr)
{
}

void pgdb_write(
    pgdb_t* db,
    const pgdb_writeoptions_t* options,
    pgdb_writebatch_t* batch,
    char** errptr)
{
}

pgdb_iterator_t* pgdb_create_iterator(
    pgdb_t* db,
    const pgdb_readoptions_t* options)
{
	return NULL;
}

const pgdb_snapshot_t* pgdb_create_snapshot(
    pgdb_t* db)
{
	return NULL;
}

void pgdb_release_snapshot(
    pgdb_t* db,
    const pgdb_snapshot_t* snapshot)
{
}

/* Returns NULL if property name is unknown.
   Else returns a pointer to a malloc()-ed null-terminated value. */
char* pgdb_property_value(
    pgdb_t* db,
    const char* propname)
{
	return NULL;
}

void pgdb_approximate_sizes(
    pgdb_t* db,
    int num_ranges,
    const char* const* range_start_key, const size_t* range_start_key_len,
    const char* const* range_limit_key, const size_t* range_limit_key_len,
    uint64_t* sizes)
{
}

void pgdb_compact_range(
    pgdb_t* db,
    const char* start_key, size_t start_key_len,
    const char* limit_key, size_t limit_key_len)
{
}

void pgdb_repair_db(
    const pgdb_options_t* options,
    const char* name,
    char** errptr)
{
}

/* Iterator */

void pgdb_iter_destroy(pgdb_iterator_t* iter)
{
}
unsigned char pgdb_iter_valid(const pgdb_iterator_t* iter)
{
	return 0xff;
}
void pgdb_iter_seek_to_first(pgdb_iterator_t* iter)
{
}
void pgdb_iter_seek_to_last(pgdb_iterator_t* iter)
{
}
void pgdb_iter_seek(pgdb_iterator_t* iter, const char* k, size_t klen)
{
}
void pgdb_iter_next(pgdb_iterator_t* iter)
{
}
void pgdb_iter_prev(pgdb_iterator_t* iter)
{
}
const char* pgdb_iter_key(const pgdb_iterator_t* iter, size_t* klen)
{
	return NULL;
}
const char* pgdb_iter_value(const pgdb_iterator_t* iter, size_t* vlen)
{
	return NULL;
}
void pgdb_iter_get_error(const pgdb_iterator_t* iter, char** errptr)
{
}

/* Write batch */

pgdb_writebatch_t* pgdb_writebatch_create(void)
{
	return NULL;
}
void pgdb_writebatch_destroy(pgdb_writebatch_t* wb)
{
}
void pgdb_writebatch_clear(pgdb_writebatch_t* wb)
{
}
void pgdb_writebatch_put(
    pgdb_writebatch_t* wb,
    const char* key, size_t klen,
    const char* val, size_t vlen)
{
}
void pgdb_writebatch_delete(
    pgdb_writebatch_t* wb,
    const char* key, size_t klen)
{
}
void pgdb_writebatch_iterate(
    pgdb_writebatch_t* wb,
    void* state,
    void (*put)(void*, const char* k, size_t klen, const char* v, size_t vlen),
    void (*deleted)(void*, const char* k, size_t klen))
{
}

/* Options */

pgdb_options_t* pgdb_options_create(void)
{
	return NULL;
}
void pgdb_options_destroy(pgdb_options_t* opt)
{
}
void pgdb_options_set_comparator(
    pgdb_options_t* opt,
    pgdb_comparator_t* cmp)
{
}
void pgdb_options_set_filter_policy(
    pgdb_options_t* opt,
    pgdb_filterpolicy_t* fp)
{
}
void pgdb_options_set_paranoid_checks(
    pgdb_options_t* opt, unsigned char yn)
{
}
void pgdb_options_set_env(pgdb_options_t* opt, pgdb_env_t* env)
{
}
void pgdb_options_set_info_log(pgdb_options_t* opt, pgdb_logger_t* lgr)
{
}
void pgdb_options_set_write_buffer_size(pgdb_options_t* opt, size_t sz)
{
}
void pgdb_options_set_max_open_files(pgdb_options_t* opt, int mof)
{
}
void pgdb_options_set_cache(pgdb_options_t* opt, pgdb_cache_t* cache)
{
}
void pgdb_options_set_block_size(pgdb_options_t* opt, size_t blksz)
{
}
void pgdb_options_set_block_restart_interval(pgdb_options_t* opt, int interval)
{
}

void pgdb_options_set_compression(pgdb_options_t* opt, int comp)
{
}

/* Comparator */

pgdb_comparator_t* pgdb_comparator_create(
    void* state,
    void (*destructor)(void*),
    int (*compare)(
        void*,
        const char* a, size_t alen,
        const char* b, size_t blen),
    const char* (*name)(void*))
{
	return NULL;
}
void pgdb_comparator_destroy(pgdb_comparator_t* cmp)
{
}

/* Filter policy */

pgdb_filterpolicy_t* pgdb_filterpolicy_create(
    void* state,
    void (*destructor)(void*),
    char* (*create_filter)(
        void*,
        const char* const* key_array, const size_t* key_length_array,
        int num_keys,
        size_t* filter_length),
    unsigned char (*key_may_match)(
        void*,
        const char* key, size_t length,
        const char* filter, size_t filter_length),
    const char* (*name)(void*))
{
	return NULL;
}
void pgdb_filterpolicy_destroy(pgdb_filterpolicy_t* fp)
{
}

pgdb_filterpolicy_t* pgdb_filterpolicy_create_bloom(
    int bits_per_key)
{
	return NULL;
}

/* Read options */

pgdb_readoptions_t* pgdb_readoptions_create(void)
{
	return NULL;
}
void pgdb_readoptions_destroy(pgdb_readoptions_t* ro)
{
}
void pgdb_readoptions_set_verify_checksums(
    pgdb_readoptions_t* ro,
    unsigned char yn)
{
}
void pgdb_readoptions_set_fill_cache(
    pgdb_readoptions_t* ro, unsigned char yn)
{
}
void pgdb_readoptions_set_snapshot(
    pgdb_readoptions_t* ro,
    const pgdb_snapshot_t* snap)
{
}

/* Write options */

pgdb_writeoptions_t* pgdb_writeoptions_create(void)
{
	return NULL;
}
void pgdb_writeoptions_destroy(pgdb_writeoptions_t* wo)
{
}
void pgdb_writeoptions_set_sync(
    pgdb_writeoptions_t* wo, unsigned char yn)
{
}

/* Cache */

pgdb_cache_t* pgdb_cache_create_lru(size_t capacity)
{
	return NULL;
}
void pgdb_cache_destroy(pgdb_cache_t* cache)
{
}

/* Env */

pgdb_env_t* pgdb_create_default_env()
{
	return NULL;
}
void pgdb_env_destroy(pgdb_env_t* env)
{
}

/* Utility */

/* Calls free(ptr).
   REQUIRES: ptr was malloc()-ed and returned by one of the routines
   in this file.  Note that in certain cases (typically on Windows), you
   may need to call this routine instead of free(ptr) to dispose of
   malloc()-ed memory returned by this library. */
void pgdb_free(void* ptr)
{
}

/* Return the major version number for this release. */
int pgdb_major_version(void)
{
	return -1;
}

/* Return the minor version number for this release. */
int pgdb_minor_version(void)
{
	return -1;
}

