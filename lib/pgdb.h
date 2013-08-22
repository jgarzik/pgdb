/* Copyright (c) 2011 The LevelDB Authors. All rights reserved.
  Use of this source code is governed by a BSD-style license that can be
  found in the LICENSE file. See the AUTHORS file for names of contributors.

  Copyright 2013 BitPay, Inc.
*/

#ifndef STORAGE_PGDB_INCLUDE_C_H_
#define STORAGE_PGDB_INCLUDE_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Exported types */

typedef struct pgdb_t               pgdb_t;
typedef struct pgdb_cache_t         pgdb_cache_t;
typedef struct pgdb_comparator_t    pgdb_comparator_t;
typedef struct pgdb_env_t           pgdb_env_t;
typedef struct pgdb_filelock_t      pgdb_filelock_t;
typedef struct pgdb_filterpolicy_t  pgdb_filterpolicy_t;
typedef struct pgdb_iterator_t      pgdb_iterator_t;
typedef struct pgdb_logger_t        pgdb_logger_t;
typedef struct pgdb_options_t       pgdb_options_t;
typedef struct pgdb_randomfile_t    pgdb_randomfile_t;
typedef struct pgdb_readoptions_t   pgdb_readoptions_t;
typedef struct pgdb_seqfile_t       pgdb_seqfile_t;
typedef struct pgdb_snapshot_t      pgdb_snapshot_t;
typedef struct pgdb_writablefile_t  pgdb_writablefile_t;
typedef struct pgdb_writebatch_t    pgdb_writebatch_t;
typedef struct pgdb_writeoptions_t  pgdb_writeoptions_t;

/* DB operations */

extern pgdb_t* pgdb_open(
    const pgdb_options_t* options,
    const char* name,
    char** errptr);

extern void pgdb_close(pgdb_t* db);

extern void pgdb_put(
    pgdb_t* db,
    const pgdb_writeoptions_t* options,
    const char* key, size_t keylen,
    const char* val, size_t vallen,
    char** errptr);

extern void pgdb_delete(
    pgdb_t* db,
    const pgdb_writeoptions_t* options,
    const char* key, size_t keylen,
    char** errptr);

extern void pgdb_write(
    pgdb_t* db,
    const pgdb_writeoptions_t* options,
    pgdb_writebatch_t* batch,
    char** errptr);

/* Returns NULL if not found.  A malloc()ed array otherwise.
   Stores the length of the array in *vallen. */
extern char* pgdb_get(
    pgdb_t* db,
    const pgdb_readoptions_t* options,
    const char* key, size_t keylen,
    size_t* vallen,
    char** errptr);

extern pgdb_iterator_t* pgdb_create_iterator(
    pgdb_t* db,
    const pgdb_readoptions_t* options);

extern const pgdb_snapshot_t* pgdb_create_snapshot(
    pgdb_t* db);

extern void pgdb_release_snapshot(
    pgdb_t* db,
    const pgdb_snapshot_t* snapshot);

/* Returns NULL if property name is unknown.
   Else returns a pointer to a malloc()-ed null-terminated value. */
extern char* pgdb_property_value(
    pgdb_t* db,
    const char* propname);

extern void pgdb_approximate_sizes(
    pgdb_t* db,
    int num_ranges,
    const char* const* range_start_key, const size_t* range_start_key_len,
    const char* const* range_limit_key, const size_t* range_limit_key_len,
    uint64_t* sizes);

extern void pgdb_compact_range(
    pgdb_t* db,
    const char* start_key, size_t start_key_len,
    const char* limit_key, size_t limit_key_len);

/* Management operations */

extern void pgdb_destroy_db(
    const pgdb_options_t* options,
    const char* name,
    char** errptr);

extern void pgdb_repair_db(
    const pgdb_options_t* options,
    const char* name,
    char** errptr);

/* Iterator */

extern void pgdb_iter_destroy(pgdb_iterator_t*);
extern unsigned char pgdb_iter_valid(const pgdb_iterator_t*);
extern void pgdb_iter_seek_to_first(pgdb_iterator_t*);
extern void pgdb_iter_seek_to_last(pgdb_iterator_t*);
extern void pgdb_iter_seek(pgdb_iterator_t*, const char* k, size_t klen);
extern void pgdb_iter_next(pgdb_iterator_t*);
extern void pgdb_iter_prev(pgdb_iterator_t*);
extern const char* pgdb_iter_key(const pgdb_iterator_t*, size_t* klen);
extern const char* pgdb_iter_value(const pgdb_iterator_t*, size_t* vlen);
extern void pgdb_iter_get_error(const pgdb_iterator_t*, char** errptr);

/* Write batch */

extern pgdb_writebatch_t* pgdb_writebatch_create();
extern void pgdb_writebatch_destroy(pgdb_writebatch_t*);
extern void pgdb_writebatch_clear(pgdb_writebatch_t*);
extern void pgdb_writebatch_put(
    pgdb_writebatch_t*,
    const char* key, size_t klen,
    const char* val, size_t vlen);
extern void pgdb_writebatch_delete(
    pgdb_writebatch_t*,
    const char* key, size_t klen);
extern void pgdb_writebatch_iterate(
    pgdb_writebatch_t*,
    void* state,
    void (*put)(void*, const char* k, size_t klen, const char* v, size_t vlen),
    void (*deleted)(void*, const char* k, size_t klen));

/* Options */

extern pgdb_options_t* pgdb_options_create();
extern void pgdb_options_destroy(pgdb_options_t*);
extern void pgdb_options_set_comparator(
    pgdb_options_t*,
    pgdb_comparator_t*);
extern void pgdb_options_set_filter_policy(
    pgdb_options_t*,
    pgdb_filterpolicy_t*);
extern void pgdb_options_set_create_if_missing(
    pgdb_options_t*, bool);
extern void pgdb_options_set_error_if_exists(
    pgdb_options_t*, bool);
extern void pgdb_options_set_paranoid_checks(
    pgdb_options_t*, unsigned char);
extern void pgdb_options_set_env(pgdb_options_t*, pgdb_env_t*);
extern void pgdb_options_set_info_log(pgdb_options_t*, pgdb_logger_t*);
extern void pgdb_options_set_write_buffer_size(pgdb_options_t*, size_t);
extern void pgdb_options_set_max_open_files(pgdb_options_t*, int);
extern void pgdb_options_set_cache(pgdb_options_t*, pgdb_cache_t*);
extern void pgdb_options_set_block_size(pgdb_options_t*, size_t);
extern void pgdb_options_set_block_restart_interval(pgdb_options_t*, int);

enum {
  pgdb_no_compression = 0,
  pgdb_snappy_compression = 1
};
extern void pgdb_options_set_compression(pgdb_options_t*, int);

/* Comparator */

extern pgdb_comparator_t* pgdb_comparator_create(
    void* state,
    void (*destructor)(void*),
    int (*compare)(
        void*,
        const char* a, size_t alen,
        const char* b, size_t blen),
    const char* (*name)(void*));
extern void pgdb_comparator_destroy(pgdb_comparator_t*);

/* Filter policy */

extern pgdb_filterpolicy_t* pgdb_filterpolicy_create(
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
    const char* (*name)(void*));
extern void pgdb_filterpolicy_destroy(pgdb_filterpolicy_t*);

extern pgdb_filterpolicy_t* pgdb_filterpolicy_create_bloom(
    int bits_per_key);

/* Read options */

extern pgdb_readoptions_t* pgdb_readoptions_create();
extern void pgdb_readoptions_destroy(pgdb_readoptions_t*);
extern void pgdb_readoptions_set_verify_checksums(
    pgdb_readoptions_t*,
    unsigned char);
extern void pgdb_readoptions_set_fill_cache(
    pgdb_readoptions_t*, unsigned char);
extern void pgdb_readoptions_set_snapshot(
    pgdb_readoptions_t*,
    const pgdb_snapshot_t*);

/* Write options */

extern pgdb_writeoptions_t* pgdb_writeoptions_create();
extern void pgdb_writeoptions_destroy(pgdb_writeoptions_t*);
extern void pgdb_writeoptions_set_sync(
    pgdb_writeoptions_t*, unsigned char);

/* Cache */

extern pgdb_cache_t* pgdb_cache_create_lru(size_t capacity);
extern void pgdb_cache_destroy(pgdb_cache_t* cache);

/* Env */

extern pgdb_env_t* pgdb_create_default_env();
extern void pgdb_env_destroy(pgdb_env_t*);

/* Utility */

/* Calls free(ptr).
   REQUIRES: ptr was malloc()-ed and returned by one of the routines
   in this file.  Note that in certain cases (typically on Windows), you
   may need to call this routine instead of free(ptr) to dispose of
   malloc()-ed memory returned by this library. */
extern void pgdb_free(void* ptr);

/* Return the major version number for this release. */
extern int pgdb_major_version();

/* Return the minor version number for this release. */
extern int pgdb_minor_version();

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif  /* STORAGE_PGDB_INCLUDE_C_H_ */
