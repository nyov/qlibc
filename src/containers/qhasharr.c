/******************************************************************************
 * qLibc - http://www.qdecoder.org
 *
 * Copyright (c) 2010-2012 Seungyoung Kim.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 * $Id$
 ******************************************************************************/

/**
 * @file qhasharr.c Static(array) hash-table implementation.
 *
 * qhasharr implements a hash-table which maps keys to values and stores into
 * fixed size static memory like shared-memory and memory-mapped file.
 * The creator qhasharr() initializes static memory to makes small slots in it.
 * The default slot size factors are defined in _Q_HASHARR_KEYSIZE and
 * _Q_HASHARR_VALUESIZE. And they are applied at compile time.
 *
 * The value part of an element will be stored across several slots if it's size
 * exceeds the slot size. But the key part of an element will be truncated if
 * the size exceeds and it's length and more complex MD5 hash value will be
 * stored with the key. So to look up a particular key, first we find an element
 * which has same hash value. If the key was not truncated, we just do key
 * comparison. But if the key was truncated because it's length exceeds, we do
 * both md5 and key comparison(only stored size) to verify that the key is same.
 * So please be aware of that, theoretically there is a possibility we pick
 * wrong element in case a key exceeds the limit, has same length and MD5 hash
 * with lookup key. But this possibility is extreamly low and almost never
 * happen in practice. If you happpen to want to make sure everything,
 * you set _Q_HASHARR_KEYSIZE big enough at compile time to make sure all keys
 * fits in it.
 *
 * qhasharr hash-table does not support thread-safe. So users should handle
 * race conditions on application side by raising user lock before calling
 * functions which modify the table data.
 *
 * @code
 *  [Data Structure Diagram]
 *
 *  +--[Static Flat Memory Area]-----------------------------------------------+
 *  | +-[Header]---------+ +-[Slot 0]---+ +-[Slot 1]---+        +-[Slot N]---+ |
 *  | |Private table data| |KEY A|DATA A| |KEY B|DATA B|  ....  |KEY N|DATA N| |
 *  | +------------------+ +------------+ +------------+        +------------+ |
 *  +--------------------------------------------------------------------------+
 *
 *  Below diagram shows how a big value is stored.
 *  +--[Static Flat Memory Area------------------------------------------------+
 *  | +--------+ +-[Slot 0]---+ +-[Slot 1]---+ +-[Slot 2]---+ +-[Slot 3]-----+ |
 *  | |TBL INFO| |KEY A|DATA A| |DATA A cont.| |KEY B|DATA B| |DATA A cont.  | |
 *  | +--------+ +------------+ +------------+ +------------+ +--------------+ |
 *  |                      ^~~link~~^     ^~~~~~~~~~link~~~~~~~~~^             |
 *  +--------------------------------------------------------------------------+
 * @endcode
 *
 * @code
 *  // initialize hash-table.
 *  char memory[1000 * 10];
 *  qhasharr_t *tbl = qhasharr(memory, sizeof(memory));
 *  if(tbl == NULL) return;
 *
 *  // insert elements (key duplication does not allowed)
 *  tbl->putstr(tbl, "e1", "a");
 *  tbl->putstr(tbl, "e2", "b");
 *  tbl->putstr(tbl, "e3", "c");
 *
 *  // debug print out
 *  tbl->debug(tbl, stdout);
 *
 *  char *e2 = tbl->getstr(tbl, "e2");
 *  if(e2 != NULL) {
 *     printf("getstr('e2') : %s\n", e2);
 *     free(e2);
 *  }
 * @endcode
 *
 * An example for using hash table over shared memory.
 *
 * @code
 *  [CREATOR SIDE]
 *  int maxslots = 1000;
 *  int memsize = qhasharr_calculate_memsize(maxslots);
 *
 *  // create shared memory
 *  int shmid = qShmInit("/tmp/some_id_file", 'q', memsize, true);
 *  if(shmid < 0) return -1; // creation failed
 *  void *memory = qShmGet(shmid);
 *
 *  // initialize hash-table
 *  qhasharr_t *tbl = qhasharr(memory, memsize);
 *  if(hasharr == NULL) return -1;
 *
 *  (...your codes with your own locking mechanism...)
 *
 *  // destroy shared memory
 *  qShmFree(shmid);
 *
 *  [USER SIDE]
 *  int shmid = qShmGetId("/tmp/some_id_file", 'q');
 *
 *  // Every table data including internal private variables are stored in the
 *  // flat static memory area. So converting the memory pointer to the
 *  // qhasharr_t pointer type is everything we need.
 *  qhasharr_t *tbl = (qhasharr_t*)qShmGet(shmid);
 *
 *  (...your codes with your own locking mechanism...)
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "qlibc.h"
#include "qinternal.h"

#ifndef _DOXYGEN_SKIP

static bool put(qhasharr_t *tbl, const char *key, const void *value,
                size_t size);
static bool putstr(qhasharr_t *tbl, const char *key, const char *str);
static bool putstrf(qhasharr_t *tbl, const char *key, const char *format, ...);
static bool putint(qhasharr_t *tbl, const char *key, int64_t num);

static void *get(qhasharr_t *tbl, const char *key, size_t *size);
static char *getstr(qhasharr_t *tbl, const char *key);
static int64_t getint(qhasharr_t *tbl, const char *key);
static bool getnext(qhasharr_t *tbl, qnobj_t *obj, int *idx);

static bool remove_(qhasharr_t *tbl, const char *key);

static int  size(qhasharr_t *tbl, int *maxslots, int *usedslots);
static void clear(qhasharr_t *tbl);
static bool debug(qhasharr_t *tbl, FILE *out);

// internal usages
static int  _find_empty(qhasharr_t *tbl, int startidx);
static int  _get_idx(qhasharr_t *tbl, const char *key, unsigned int hash);
static void *_get_data(qhasharr_t *tbl, int idx, size_t *size);
static bool _put_data(qhasharr_t *tbl, int idx, unsigned int hash,
                      const char *key, const void *value, size_t size,
                      int count);
static bool _copy_slot(qhasharr_t *tbl, int idx1, int idx2);
static bool _remove_slot(qhasharr_t *tbl, int idx);
static bool _remove_data(qhasharr_t *tbl, int idx);

#endif

/**
 * Get how much memory is needed for N slots.
 *
 * @param max       a number of maximum internal slots
 *
 * @return memory size needed
 *
 * @note
 *  This can be used for calculating minimum memory size for N slots.
 */
size_t qhasharr_calculate_memsize(int max)
{
    size_t memsize = sizeof(qhasharr_t)
                     + (sizeof(struct _Q_HASHARR_SLOT) * (max));
    return memsize;
}

/**
 * Initialize static hash table
 *
 * @param memory    a pointer of buffer memory.
 * @param memsize   a size of buffer memory.
 *
 * @return qhasharr_t container pointer. structure(same as buffer pointer),
 *  otherwise returns NULL.
 * @retval errno  will be set in error condition.
 *  - EINVAL : Assigned memory is too small. It must bigger enough to allocate
 *  at least 1 slot.
 *
 * @code
 *  // initialize hash-table with 100 slots.
 *  // A single element can take several slots.
 *  char memory[112 * 100];
 *  qhasharr_t *tbl = qhasharr(memory, sizeof(memory));
 * @endcode
 *
 * @note
 *  Every information is stored in user memory. So the returning container
 *  pointer exactly same as memory pointer.
 */
qhasharr_t *qhasharr(void *memory, size_t memsize)
{
    // calculate max
    int maxslots = (memsize - sizeof(qhasharr_t))
                   / sizeof(struct _Q_HASHARR_SLOT);
    if (maxslots < 1 || memsize <= sizeof(qhasharr_t)) {
        errno = EINVAL;
        return NULL;
    }

    // clear memory
    qhasharr_t *tbl = (qhasharr_t *)memory;
    memset((void *)tbl, 0, memsize);

    tbl->maxslots = maxslots;
    tbl->usedslots = 0;
    tbl->num = 0;

    tbl->slots = (struct _Q_HASHARR_SLOT *)(memory + sizeof(qhasharr_t));

    // assign methods
    tbl->put        = put;
    tbl->putstr     = putstr;
    tbl->putstrf    = putstrf;
    tbl->putint     = putint;

    tbl->get        = get;
    tbl->getstr     = getstr;
    tbl->getint     = getint;
    tbl->getnext    = getnext;

    tbl->remove     = remove_;

    tbl->size       = size;
    tbl->clear      = clear;
    tbl->debug      = debug;

    return (qhasharr_t *)memory;
}

/**
 * qhasharr->put(): Put an object into this table.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 * @param value     value object data
 * @param size      size of value
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Table doesn't have enough space to store the object.
 *  - EINVAL    : Invalid argument.
 *  - EFAULT    : Unexpected error. Data structure is not constant.
*/
static bool put(qhasharr_t *tbl, const char *key, const void *value,
                size_t size)
{
    if (key == NULL || value == NULL) {
        errno = EINVAL;
        return false;
    }

    // check full
    if (tbl->usedslots >= tbl->maxslots) {
        DEBUG("hasharr: put %s - FULL", key);
        errno = ENOBUFS;
        return false;
    }

    // get hash integer
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;

    // check, is slot empty
    if (tbl->slots[hash].count == 0) { // empty slot
        // put data
        if (_put_data(tbl, hash, hash, key, value, size, 1) == false) {
            DEBUG("hasharr: FAILED put(new) %s", key);
            return false;
        }
        DEBUG("hasharr: put(new) %s (idx=%d,hash=%u,tot=%d)",
              key, hash, hash, tbl->usedslots);
    } else if (tbl->slots[hash].count > 0) { // same key or hash collision
        // check same key;
        int idx = _get_idx(tbl, key, hash);
        if (idx >= 0) { // same key
            // remove and recall
            remove_(tbl, key);
            return put(tbl, key, value, size);
        } else { // no same key, just hash collision
            // find empty slot
            int idx = _find_empty(tbl, hash);
            if (idx < 0) {
                errno = ENOBUFS;
                return false;
            }

            // put data. -1 is used for collision resolution (idx != hash);
            if (_put_data(tbl, idx, hash, key, value, size, -1) == false) {
                DEBUG("hasharr: FAILED put(col) %s", key);
                return false;
            }

            // increase counter from leading slot
            tbl->slots[hash].count++;

            DEBUG("hasharr: put(col) %s (idx=%d,hash=%u,tot=%d)",
                  key, idx, hash, tbl->usedslots);
        }
    } else {
        // in case of -1 or -2, move it. -1 used for collision resolution,
        // -2 used for oversized value data.

        // find empty slot
        int idx = _find_empty(tbl, hash + 1);
        if (idx < 0) {
            errno = ENOBUFS;
            return false;
        }

        // move dup slot to empty
        _copy_slot(tbl, idx, hash);
        _remove_slot(tbl, hash);

        // in case of -2, adjust link of mother
        if (tbl->slots[idx].count == -2) {
            tbl->slots[ tbl->slots[idx].hash ].link = idx;
            if (tbl->slots[idx].link != -1) {
                tbl->slots[ tbl->slots[idx].link ].hash = idx;
            }
        }

        // store data
        if (_put_data(tbl, hash, hash, key, value, size, 1) == false) {
            DEBUG("hasharr: FAILED put(swp) %s", key);
            return false;
        }

        DEBUG("hasharr: put(swp) %s (idx=%u,hash=%u,tot=%d)",
              key, hash, hash, tbl->usedslots);
    }

    return true;
}

/**
 * qhasharr->putstr(): Put a string into this table
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string.
 * @param value     string data.
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Table doesn't have enough space to store the object.
 *  - EINVAL    : Invalid argument.
 *  - EFAULT    : Unexpected error. Data structure is not constant.
 */
static bool putstr(qhasharr_t *tbl, const char *key, const char *str)
{
    int size = (str != NULL) ? (strlen(str) + 1) : 0;
    return put(tbl, key, (void *)str, size);
}

/**
 * qhasharr->putstrf(): Put a formatted string into this table.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key name.
 * @param format    formatted string data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Table doesn't have enough space to store the object.
 *  - ENOMEM    : System memory allocation failure.
 *  - EINVAL    : Invalid argument.
 *  - EFAULT    : Unexpected error. Data structure is not constant.
 */
static bool putstrf(qhasharr_t *tbl, const char *key, const char *format, ...)
{
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = putstr(tbl, key, str);
    free(str);
    return ret;
}

/**
 * qhasharr->putint(): Put an integer into this table as string type.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 * @param value     value integer
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Table doesn't have enough space to store the object.
 *  - EINVAL    : Invalid argument.
 *  - EFAULT    : Unexpected error. Data structure is not constant.
 *
 * @note
 * The integer will be converted to a string object and stored as string object.
 */
static bool putint(qhasharr_t *tbl, const char *key, int64_t num)
{
    char str[20+1];
    snprintf(str, sizeof(str), "%"PRId64, num);
    return putstr(tbl, key, str);
}

/**
 * qhasharr->get(): Get an object from this table
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 * @param size      if not NULL, oject size will be stored
 *
 * @return malloced object pointer if successful, otherwise(not found)
 *  returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failed.
 *
 * @note
 * returned object must be freed after done using.
 */
static void *get(qhasharr_t *tbl, const char *key, size_t *size)
{
    if (key == NULL) {
        errno = EINVAL;
        return NULL;
    }

    // get hash integer
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;

    int idx = _get_idx(tbl, key, hash);
    if (idx < 0) {
        errno = ENOENT;
        return NULL;
    }

    return _get_data(tbl, idx, size);
}

/**
 * qhasharr->getstr(): Finds an object with given name and returns as
 * string type.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 *
 * @return string pointer if successful, otherwise(not found) returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failed.
 *
 * @note
 * returned object must be freed after done using.
 */
static char *getstr(qhasharr_t *tbl, const char *key)
{
    return (char *)get(tbl, key, NULL);
}

/**
 * qhasharr->getint(): Finds an object with given name and returns as
 * integer type.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 *
 * @return value integer if successful, otherwise(not found) returns 0
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failed.
 */
static int64_t getint(qhasharr_t *tbl, const char *key)
{
    int64_t num = 0;
    char *str = getstr(tbl, key);
    if (str != NULL) {
        num = atoll(str);
        free(str);
    }

    return num;
}

/**
 * qhasharr->getnext(): Get next element.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param idx       index pointer
 *
 * @return key name string if successful, otherwise(end of table) returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : No next element.
 *  - EINVAL    : Invald argument.
 *  - ENOMEM    : Memory allocation failed.
 *
 * @code
 *  int idx = 0;
 *  qnobj_t obj;
 *  while(tbl->getnext(tbl, &obj, &idx) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *    free(obj.name);
 *    free(obj.data);
 *  }
 * @endcode
 *
 * @note
 *  Please be aware a key name will be returned with truncated length
 *  because key name is truncated when it put into the table if it's length is
 *  longer than _Q_HASHARR_KEYSIZE.
 */
static bool getnext(qhasharr_t *tbl, qnobj_t *obj, int *idx)
{
    if (obj == NULL || idx == NULL) {
        errno = EINVAL;
        return NULL;
    }

    for (; *idx < tbl->maxslots; (*idx)++) {
        if (tbl->slots[*idx].count == 0 || tbl->slots[*idx].count == -2) {
            continue;
        }

        size_t keylen = tbl->slots[*idx].data.pair.keylen;
        if (keylen > _Q_HASHARR_KEYSIZE) keylen = _Q_HASHARR_KEYSIZE;

        obj->name = (char *)malloc(keylen + 1);
        if (obj->name == NULL) {
            errno = ENOMEM;
            return false;
        }
        memcpy(obj->name, tbl->slots[*idx].data.pair.key, keylen);
        obj->name[keylen] = '\0';

        obj->data = _get_data(tbl, *idx, &obj->size);
        if (obj->data == NULL) {
            free(obj->name);
            errno = ENOMEM;
            return false;
        }

        *idx += 1;
        return true;
    }

    errno = ENOENT;
    return false;
}

/**
 * qhasharr->remove(): Remove an object from this table.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 *
 * @return true if successful, otherwise(not found) returns false
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invald argument.
 *  - EFAULT        : Unexpected error. Data structure is not constant.
 */
static bool remove_(qhasharr_t *tbl, const char *key)
{
    if (key == NULL) {
        errno = EINVAL;
        return false;
    }

    // get hash integer
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;

    int idx = _get_idx(tbl, key, hash);
    if (idx < 0) {
        DEBUG("not found %s", key);
        errno = ENOENT;
        return false;
    }

    if (tbl->slots[idx].count == 1) {
        // just remove
        _remove_data(tbl, idx);
        DEBUG("hasharr: rem %s (idx=%d,tot=%d)", key, idx, tbl->usedslots);
    } else if (tbl->slots[idx].count > 1) { // leading slot and has dup
        // find dup
        int idx2;
        for (idx2 = idx + 1; ; idx2++) {
            if (idx2 >= tbl->maxslots) idx2 = 0;
            if (idx2 == idx) {
                DEBUG("hasharr: [BUG] failed to remove dup key %s.", key);
                errno = EFAULT;
                return false;
            }
            if (tbl->slots[idx2].count == -1 && tbl->slots[idx2].hash == hash) {
                break;
            }
        }

        // move to leading slot
        int backupcount = tbl->slots[idx].count;
        _remove_data(tbl, idx); // remove leading data
        _copy_slot(tbl, idx, idx2); // copy slot
        _remove_slot(tbl, idx2); // remove moved slot

        tbl->slots[idx].count = backupcount - 1; // adjust collision counter
        if (tbl->slots[idx].link != -1) {
            tbl->slots[tbl->slots[idx].link].hash = idx;
        }

        DEBUG("hasharr: rem(lead) %s (idx=%d,tot=%d)",
              key, idx, tbl->usedslots);
    } else { // in case of -1. used for collision resolution
        // decrease counter from leading slot
        if (tbl->slots[ tbl->slots[idx].hash ].count <= 1) {
            DEBUG("hasharr: [BUG] failed to remove  %s. "
                  "counter of leading slot mismatch.", key);
            errno = EFAULT;
            return false;
        }
        tbl->slots[ tbl->slots[idx].hash ].count--;

        // remove data
        _remove_data(tbl, idx);
        DEBUG("hasharr: rem(dup) %s (idx=%d,tot=%d)", key, idx, tbl->usedslots);
    }

    return true;
}

/**
 * qhasharr->size(): Returns the number of objects in this table.
 *
 * @param tbl       qhasharr_t container pointer.
 *
 * @return a number of elements stored
 */
static int size(qhasharr_t *tbl, int *maxslots, int *usedslots)
{
    if (tbl == NULL) return false;

    if (maxslots != NULL) *maxslots = tbl->maxslots;
    if (usedslots != NULL) *usedslots = tbl->usedslots;

    return tbl->num;
}

/**
 * qhasharr->clear(): Clears this table so that it contains no keys.
 *
 * @param tbl       qhasharr_t container pointer.
 *
 * @return true if successful, otherwise returns false
 */
static void clear(qhasharr_t *tbl)
{
    if (tbl->usedslots == 0) return;

    tbl->usedslots = 0;
    tbl->num = 0;

    // clear memory
    memset((void *)tbl->slots,
           '\0',
           (tbl->maxslots * sizeof(struct _Q_HASHARR_SLOT)));
}

/**
 * qhasharr->debug(): Print hash table for debugging purpose
 *
 * @param tbl       qhasharr_t container pointer.
 * @param out       output stream
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EIO       : Invalid output stream.
 */
static bool debug(qhasharr_t *tbl, FILE *out)
{
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    int idx = 0;
    qnobj_t obj;
    while (tbl->getnext(tbl, &obj, &idx) == true) {
        uint16_t keylen = tbl->slots[idx-1].data.pair.keylen;
        fprintf(out, "%s%s(%d)=" ,
                obj.name, (keylen>_Q_HASHARR_KEYSIZE)?"...":"", keylen);
        _q_humanOut(out, obj.data, obj.size, MAX_HUMANOUT);
        fprintf(out, " (%zu)\n" , obj.size);

        free(obj.name);
        free(obj.data);
    }

#ifdef BUILD_DEBUG
    fprintf(out, "%d elements (slot %d used/%d total)\n",
            tbl->num, tbl->usedslots, tbl->maxslots);
    for (idx = 0; idx < tbl->maxslots; idx++) {
        if (tbl->slots[idx].count == 0) continue;

        fprintf(out, "slot=%d,type=", idx);
        if (tbl->slots[idx].count == -2) {
            fprintf(out, "EXTEND,prev=%d,next=%d,data=",
                    tbl->slots[idx].hash,  tbl->slots[idx].link);
            _q_humanOut(out,
                        tbl->slots[idx].data.ext.value,
                        tbl->slots[idx].size,
                        MAX_HUMANOUT);
            fprintf(out, ",size=%d", tbl->slots[idx].size);
        } else {
            fprintf(out, "%s", (tbl->slots[idx].count == -1)?"COLISN":"NORMAL");
            fprintf(out, ",count=%d,hash=%u,key=",
                    tbl->slots[idx].count, tbl->slots[idx].hash);
            _q_humanOut(out,
                        tbl->slots[idx].data.pair.key,
                        (tbl->slots[idx].data.pair.keylen>_Q_HASHARR_KEYSIZE)
                        ? _Q_HASHARR_KEYSIZE
                        : tbl->slots[idx].data.pair.keylen,
                        MAX_HUMANOUT);
            fprintf(out, ",keylen=%d,data=", tbl->slots[idx].data.pair.keylen);
            _q_humanOut(out,
                        tbl->slots[idx].data.pair.value,
                        tbl->slots[idx].size,
                        MAX_HUMANOUT);
            fprintf(out, ",size=%d", tbl->slots[idx].size);
        }
        fprintf(out, "\n");
    }
#endif

    return true;
}

#ifndef _DOXYGEN_SKIP

// find empty slot : return empty slow number, otherwise returns -1.
static int _find_empty(qhasharr_t *tbl, int startidx)
{
    if (startidx >= tbl->maxslots) startidx = 0;

    int idx = startidx;
    while (true) {
        if (tbl->slots[idx].count == 0) return idx;

        idx++;
        if (idx >= tbl->maxslots) idx = 0;
        if (idx == startidx) break;
    }

    return -1;
}

static int _get_idx(qhasharr_t *tbl, const char *key, unsigned int hash)
{
    if (tbl->slots[hash].count > 0) {
        int count, idx;
        for (count = 0, idx = hash; count < tbl->slots[hash].count; ) {
            if (tbl->slots[idx].hash == hash
                && (tbl->slots[idx].count > 0 || tbl->slots[idx].count == -1)) {
                // same hash
                count++;

                // is same key?
                size_t keylen = strlen(key);
                // first check key length
                if (keylen == tbl->slots[idx].data.pair.keylen) {
                    if (keylen <= _Q_HASHARR_KEYSIZE) {
                        // original key is stored
                        if (!memcmp(key, tbl->slots[idx].data.pair.key,
                                    keylen)) {
                            return idx;
                        }
                    } else {
                        // key is truncated, compare MD5 also.
                        unsigned char keymd5[16];
                        qhashmd5(key, keylen, keymd5);
                        if (!memcmp(key, tbl->slots[idx].data.pair.key,
                                    _Q_HASHARR_KEYSIZE) &&
                            !memcmp(keymd5, tbl->slots[idx].data.pair.keymd5,
                                    16)) {
                            return idx;
                        }
                    }
                }
            }

            // increase idx
            idx++;
            if (idx >= tbl->maxslots) idx = 0;

            // check loop
            if (idx == hash) break;

            continue;
        }
    }

    return -1;
}

static void *_get_data(qhasharr_t *tbl, int idx, size_t *size)
{
    if (idx < 0) {
        errno = ENOENT;
        return NULL;
    }

    int newidx;
    size_t valsize;
    for (newidx = idx, valsize = 0; ; newidx = tbl->slots[newidx].link) {
        valsize += tbl->slots[newidx].size;
        if (tbl->slots[newidx].link == -1) break;
    }

    void *value, *vp;
    value = malloc(valsize);
    if (value == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    for (newidx = idx, vp = value; ; newidx = tbl->slots[newidx].link) {
        if (tbl->slots[newidx].count == -2) {
            // extended data block
            memcpy(vp, (void *)tbl->slots[newidx].data.ext.value,
                   tbl->slots[newidx].size);
        } else {
            // key/value pair data block
            memcpy(vp, (void *)tbl->slots[newidx].data.pair.value,
                   tbl->slots[newidx].size);
        }

        vp += tbl->slots[newidx].size;
        if (tbl->slots[newidx].link == -1) break;
    }

    if (size != NULL) *size = valsize;
    return value;
}

static bool _put_data(qhasharr_t *tbl, int idx, unsigned int hash,
                      const char *key, const void *value, size_t size,
                      int count)
{
    // check if used
    if (tbl->slots[idx].count != 0) {
        DEBUG("hasharr: BUG found.");
        errno = EFAULT;
        return false;
    }

    size_t keylen = strlen(key);
    unsigned char keymd5[16];
    qhashmd5(key, keylen, keymd5);

    // store key
    tbl->slots[idx].count = count;
    tbl->slots[idx].hash = hash;
    strncpy(tbl->slots[idx].data.pair.key, key, _Q_HASHARR_KEYSIZE);
    memcpy((char *)tbl->slots[idx].data.pair.keymd5, (char *)keymd5, 16);
    tbl->slots[idx].data.pair.keylen = keylen;
    tbl->slots[idx].link = -1;

    // store value
    int newidx;
    size_t savesize;
    for (newidx = idx, savesize = 0; savesize < size;) {
        if (savesize > 0) { // find next empty slot
            int tmpidx = _find_empty(tbl, newidx + 1);
            if (tmpidx < 0) {
                DEBUG("hasharr: Can't expand slot for key %s.", key);
                _remove_data(tbl, idx);
                errno = ENOBUFS;
                return false;
            }

            // clear & set
            memset((void *)(&tbl->slots[tmpidx]), '\0',
                   sizeof(struct _Q_HASHARR_SLOT));

            tbl->slots[tmpidx].count = -2;      // extended data block
            tbl->slots[tmpidx].hash = newidx;   // prev link
            tbl->slots[tmpidx].link = -1;       // end block mark
            tbl->slots[tmpidx].size = 0;

            tbl->slots[newidx].link = tmpidx;   // link chain

            DEBUG("hasharr: slot %d is linked to slot %d for key %s.",
                  tmpidx, newidx, key);
            newidx = tmpidx;
        }

        // copy data
        size_t copysize = size - savesize;

        if (tbl->slots[newidx].count == -2) {
            // extended value
            if (copysize > sizeof(struct _Q_HASHARR_SLOT_EXT)) {
                copysize = sizeof(struct _Q_HASHARR_SLOT_EXT);
            }
            memcpy(tbl->slots[newidx].data.ext.value,
                   value + savesize, copysize);
        } else {
            // first slot
            if (copysize > _Q_HASHARR_VALUESIZE) {
                copysize = _Q_HASHARR_VALUESIZE;
            }
            memcpy(tbl->slots[newidx].data.pair.value,
                   value + savesize, copysize);

            // increase stored key counter
            tbl->num++;
        }
        tbl->slots[newidx].size = copysize;
        savesize += copysize;

        // increase used slot counter
        tbl->usedslots++;
    }

    return true;
}

static bool _copy_slot(qhasharr_t *tbl, int idx1, int idx2)
{
    if (tbl->slots[idx1].count != 0 || tbl->slots[idx2].count == 0) {
        DEBUG("hasharr: BUG found.");
        errno = EFAULT;
        return false;
    }

    memcpy((void *)(&tbl->slots[idx1]), (void *)(&tbl->slots[idx2]),
           sizeof(struct _Q_HASHARR_SLOT));

    // increase used slot counter
    tbl->usedslots++;

    return true;
}

static bool _remove_slot(qhasharr_t *tbl, int idx)
{
    if (tbl->slots[idx].count == 0) {
        DEBUG("hasharr: BUG found.");
        errno = EFAULT;
        return false;
    }

    tbl->slots[idx].count = 0;

    // decrease used slot counter
    tbl->usedslots--;

    return true;
}

static bool _remove_data(qhasharr_t *tbl, int idx)
{
    if (tbl->slots[idx].count == 0) {
        DEBUG("hasharr: BUG found.");
        errno = EFAULT;
        return false;
    }

    while (true) {
        int link = tbl->slots[idx].link;
        _remove_slot(tbl, idx);

        if (link == -1) break;

        idx = link;
    }

    // decrease stored key counter
    tbl->num--;

    return true;
}

#endif /* _DOXYGEN_SKIP */
