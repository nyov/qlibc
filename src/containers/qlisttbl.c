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
 * @file qlisttbl.c Linked-list-table implementation.
 *
 * qlisttbl_t container is a Linked-List-Table implementation.
 * Which maps keys to values. Key is a string and value is any non-null object.
 * These elements are stored sequentially in Doubly-Linked-List data structure.
 * In addition, qlisttbl_t allows to add multiple records with same keys.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *  last~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
 *                                                          |
 *          +-----------+  doubly  +-----------+  doubly  +-|---------+
 *  first~~~|~>   0   <~|~~~~~~~~~~|~>   1   <~|~~~~~~~~~~|~>   N     |
 *          +--|-----|--+  linked  +--|-----|--+  linked  +--|-----|--+
 *             |     |                |     |                |     |
 *       +-----v-+ +-v------------+   |     |          +-----v-+ +-v-------+
 *       | KEY A | | VALUE A      |   |     |          | KEY N | | VALUE N |
 *       +-------+ +--------------+   |     |          +-------+ +---------+
 *                 +------------------v-+ +-v---------------------+
 *                 | KEY B              | | VALUE B               |
 *                 +--------------------+ +-----------------------+
 * @endcode
 *
 * @code
 *  // create a list.
 *  qlisttbl_t *tbl = qlisttbl();
 *
 *  // insert elements (key duplication allowed)
 *  tbl->put(tbl, "e1", "a", strlen("e1")+1, false); // equal to addStr();
 *  tbl->put_str(tbl, "e2", "b", false);
 *  tbl->put_str(tbl, "e2", "c", false);
 *  tbl->put_str(tbl, "e3", "d", false);
 *
 *  // debug output
 *  tbl->debug(tbl, stdout, true);
 *
 *  // get element
 *  printf("get('e2') : %s\n", (char*)tbl->get(tbl, "e2", NULL, false));
 *  printf("get_last('e2') : %s\n",
 *         (char*)tbl->get_last(tbl, "e2", NULL, false));
 *  printf("get_str('e2') : %s\n", tbl->get_str(tbl, "e2", false));
 *
 *  // find every 'e2' elements
 *  qdlnobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  while(tbl->get_next(tbl, &obj, "e2", false) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *  }
 *
 *  // free object
 *  tbl->terminate(tbl);
 * @endcode
 *
 * @note
 *  Use "--enable-threadsafe" configure script option to use under
 *  multi-threaded environments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "qlibc.h"
#include "qInternal.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP

#define _VAR '$'
#define _VAR_OPEN '{'
#define _VAR_CLOSE '}'
#define _VAR_CMD '!'
#define _VAR_ENV '%'

static bool put(qlisttbl_t *tbl, const char *name, const void *data,
                size_t size, bool unique);

static bool put_first(qlisttbl_t *tbl, const char *name, const void *data,
                      size_t size, bool unique);
static bool put_last(qlisttbl_t *tbl, const char *name, const void *data,
                     size_t size, bool unique);

static bool put_str(qlisttbl_t *tbl, const char *name, const char *str,
                    bool unique);
static bool put_strf(qlisttbl_t *tbl, bool unique, const char *name,
                     const char *format, ...);
static bool put_int(qlisttbl_t *tbl, const char *name, int num, bool unique);

static void *get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem);
static void *get_last(qlisttbl_t *tbl, const char *name, size_t *size,
                      bool newmem);

static char *get_str(qlisttbl_t *tbl, const char *name, bool newmem);
static int get_int(qlisttbl_t *tbl, const char *name);

static void *caseget(qlisttbl_t *tbl, const char *name, size_t *size,
                     bool newmem);
static char *caseget_str(qlisttbl_t *tbl, const char *name, bool newmem);
static int caseget_int(qlisttbl_t *tbl, const char *name);

static bool get_next(qlisttbl_t *tbl, qdlnobj_t *obj, const char *name,
                     bool newmem);

static bool remove_(qlisttbl_t *tbl, const char *name);

static bool set_putdir(qlisttbl_t *tbl, bool first);
static size_t size(qlisttbl_t *tbl);
static void reverse(qlisttbl_t *tbl);
static void clear(qlisttbl_t *tbl);

static char *parse_str(qlisttbl_t *tbl, const char *str);
static bool save(qlisttbl_t *tbl, const char *filepath, char sepchar,
                 bool encode);
static size_t load(qlisttbl_t *tbl, const char *filepath, char sepchar,
                   bool decode);
static bool debug(qlisttbl_t *tbl, FILE *out);

static void lock(qlisttbl_t *tbl);
static void unlock(qlisttbl_t *tbl);

static void terminate(qlisttbl_t *tbl);

/* internal functions */
static bool _put(qlisttbl_t *tbl, const char *name, const void *data,
                 size_t size, bool unique, bool first);
static void *_get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem,
                  bool backward, int (*cmp)(const char *s1, const char *s2));

#endif

/**
 * Create a new Q_LIST linked-list container
 *
 * @return a pointer of malloced qlisttbl_t structure in case of successful,
 *  otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qlisttbl_t *tbl = qlisttbl();
 * @endcode
 */
qlisttbl_t *qlisttbl(void)
{
    qlisttbl_t *tbl = (qlisttbl_t *)malloc(sizeof(qlisttbl_t));
    if (tbl == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // initialize container
    memset((void *)tbl, 0, sizeof(qlisttbl_t));

    // member methods
    tbl->put        = put;
    tbl->put_first  = put_first;
    tbl->put_last   = put_last;

    tbl->put_str    = put_str;
    tbl->put_strf   = put_strf;
    tbl->put_int    = put_int;

    tbl->get        = get;
    tbl->get_last   = get_last;

    tbl->get_str    = get_str;
    tbl->get_int    = get_int;

    tbl->caseget    = caseget;
    tbl->caseget_str = caseget_str;
    tbl->caseget_int = caseget_int;

    tbl->get_next   = get_next;

    tbl->remove     = remove_;

    tbl->set_putdir = set_putdir;
    tbl->size       = size;
    tbl->reverse    = reverse;
    tbl->clear      = clear;

    tbl->parse_str  = parse_str;
    tbl->save       = save;
    tbl->load       = load;
    tbl->debug      = debug;

    tbl->lock       = lock;
    tbl->unlock     = unlock;

    tbl->terminate  = terminate;

    // initialize recrusive mutex
    Q_MUTEX_INIT(tbl->qmutex, true);

    return tbl;
}

/**
 * (qlisttbl_t*)->put(): Put an element to this table.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param name      element name.
 * @param data      a pointer which points data memory.
 * @param size      size of the data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @code
 *  struct my_obj {
 *    int a, b, c;
 *  };
 *
 *  // create a sample object.
 *  struct my_obj obj;
 *
 *  // create a list and add the sample object.
 *  qlisttbl_t *tbl = qlisttbl();
 *  tbl->put(tbl, "obj1", &obj, sizeof(struct my_obj), false);
 * @endcode
 *
 * @note
 *  The default behavior is adding object at the end of this table unless it's
 *  changed by calling setPutDirection().
 */
static bool put(qlisttbl_t *tbl, const char *name, const void *data,
                size_t size, bool unique)
{
    return _put(tbl, name, data, size, unique, tbl->putdir);
}

/**
 * (qlisttbl_t*)->putFirst(): Put an element at the beginning of this table.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param name      element name.
 * @param data      a pointer which points data memory.
 * @param size      size of the data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 */
static bool put_first(qlisttbl_t *tbl, const char *name, const void *data,
                      size_t size, bool unique)
{
    return _put(tbl, name, data, size, unique, true);
}

/**
 * (qlisttbl_t*)->putLast(): Put an object at the end of this table.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param name      element name.
 * @param data      a pointer which points data memory.
 * @param size      size of the data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 */
static bool put_last(qlisttbl_t *tbl, const char *name, const void *data,
                     size_t size, bool unique)
{
    return _put(tbl, name, data, size, unique, false);
}

/**
 * (qlisttbl_t*)->put_str(): Put a string into this table.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param name      element name.
 * @param str       string data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @note
 *  The default behavior is adding object at the end of this list unless it's
 *  changed by calling setDirection().
 */
static bool put_str(qlisttbl_t *tbl, const char *name, const char *str,
                    bool unique)
{
    size_t size = (str!=NULL) ? (strlen(str) + 1) : 0;
    return put(tbl, name, (const void *)str, size, unique);
}

/**
 * (qlisttbl_t*)->put_strf(): Put a formatted string into this table.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 * @param name      element name.
 * @param format    formatted value string.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @note
 *  The default behavior is adding object at the end of this list unless it's
 *  changed by calling setDirection().
 */
static bool put_strf(qlisttbl_t *tbl, bool unique, const char *name,
                     const char *format, ...)
{
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = put_str(tbl, name, str, unique);
    free(str);

    return ret;
}

/**
 * (qlisttbl_t*)->putInt(): Put an integer into this table as string type.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param name      element name.
 * @param num       number data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @note
 *  The integer will be converted to a string object and stored as a string
 *  object. The default behavior is adding object at the end of this list unless
 *  it's changed by calling setDirection().
 */
static bool put_int(qlisttbl_t *tbl, const char *name, int num, bool unique)
{
    char str[20+1];
    snprintf(str, sizeof(str), "%d", num);
    return put_str(tbl, name, str, unique);
}

/**
 * (qlisttbl_t*)->get(): Finds an object with given name.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param name      element name.
 * @param size      if size is not NULL, data size will be stored.
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qlisttbl_t *tbl = qlisttbl();
 *  (...codes...)
 *
 *  // with newmem flag unset
 *  size_t size;
 *  void *data = tbl->get(tbl, "key_name", &size, false);
 *
 *  // with newmem flag set
 *  size_t size;
 *  void *data = tbl->get(tbl, "key_name", &size, true);
 *  (...does something with data...)
 *  if(data != NULL) free(data);
 * @endcode
 *
 * @note
 *  If newmem flag is set, returned data will be malloced and should be
 *  deallocated by user. Otherwise returned pointer will point internal data
 *  buffer directly and should not be de-allocated by user. In thread-safe mode,
 *  always set newmem flag as true to make sure it works in race condition
 *  situation.
 */
static void *get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem)
{
    return _get(tbl, name, size, newmem, false, strcmp);
}

/**
 * (qlisttbl_t*)->get_last(): Finds an object backward from the last of list
 * with given name.
 *
 * @param tbl qlisttbl_t container pointer.
 * @param name element name.
 * @param size if size is not NULL, data size will be stored.
 * @param newmem whether or not to allocate memory for the data.
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 *  This can be used for find last matched element if there are multiple objects
 *  with same name.
 */
static void *get_last(qlisttbl_t *tbl, const char *name, size_t *size,
                      bool newmem)
{
    return _get(tbl, name, size, newmem, true, strcmp);
}

/**
 * (qlisttbl_t*)->get_str():  Finds an object with given name and returns as
 * string type.
 *
 * @param tbl qlisttbl_t container pointer.
 * @param name element name.
 * @param newmem whether or not to allocate memory for the data.
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
  * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
*/
static char *get_str(qlisttbl_t *tbl, const char *name, bool newmem)
{
    return (char *)get(tbl, name, NULL, newmem);
}

/**
 * (qlisttbl_t*)->get_int():  Finds an object with given name and returns as
 * integer type.
 *
 * @param tbl qlisttbl_t container pointer.
 * @param name element name.
 *
 * @return an integer value of the integer object, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static int get_int(qlisttbl_t *tbl, const char *name)
{
    int num = 0;
    char *str = get_str(tbl, name, true);
    if (str != NULL) {
        num = atoi(str);
        free(str);
    }
    return num;
}

/**
 * (qlisttbl_t*)->caseget(): Finds an object with given name. (case-insensitive)
 *
 * @param tbl qlisttbl_t container pointer.
 * @param name element name.
 * @param size if size is not NULL, data size will be stored.
 * @param newmem whether or not to allocate memory for the data.
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static void *caseget(qlisttbl_t *tbl, const char *name, size_t *size,
                     bool newmem)
{
    return _get(tbl, name, size, newmem, false, strcasecmp);
}

/**
 * (qlisttbl_t*)->caseget_str(): same as get_str() but case-insensitive.
 *
 * @param tbl qlisttbl_t container pointer.
 * @param name element name.
 * @param newmem whether or not to allocate memory for the data.
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static char *caseget_str(qlisttbl_t *tbl, const char *name, bool newmem)
{
    return (char *)caseget(tbl, name, NULL, newmem);
}

/**
 * (qlisttbl_t*)->caseget_int(): same as get_int() but case-insensitive.
 *
 * @param tbl qlisttbl_t container pointer.
 * @param name element name.
 *
 * @return an integer value of the integer object, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static int caseget_int(qlisttbl_t *tbl, const char *name)
{
    int num = 0;
    char *str = caseget(tbl, name, NULL, true);
    if (str != NULL) {
        num = atoi(str);
        free(str);
    }
    return num;
}

/**
 * (qlisttbl_t*)->get_next(): Get next element.
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param obj       found data will be stored in this object
 * @param name      element name., if key name is NULL search every objects in
 *                  the list.
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return true if found otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOENT : No next element.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 *  obj should be initialized with 0 by using memset() before first call.
 *  If newmem flag is true, user should de-allocate obj.name and obj.data
 *  resources.
 *
 * @code
 *  qlisttbl_t *tbl = qlisttbl();
 *  (...add data into list...)
 *
 *  // non-thread usages
 *  qdlnobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  while(tbl->get_next(tbl, &obj, NULL, false) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *  }
 *
 *  // thread model with particular key search
 *  qdlnobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);
 *  while(tbl->get_next(tbl, &obj, "key_name", true) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *    free(obj.name);
 *    free(obj.data);
 *  }
 *  tbl->unlock(tbl);
 * @endcode
 */
static bool get_next(qlisttbl_t *tbl, qdlnobj_t *obj, const char *name,
                     bool newmem)
{
    if (obj == NULL) return NULL;

    lock(tbl);

    qdlnobj_t *cont = NULL;
    if (obj->size == 0) cont = tbl->first;
    else cont = obj->next;

    if (cont == NULL) {
        errno = ENOENT;
        unlock(tbl);
        return false;
    }

    bool ret = false;
    for (; cont != NULL; cont = cont->next) {
        if (name == NULL || !strcmp(cont->name, name)) {
            if (newmem == true) {
                obj->name = strdup(cont->name);
                obj->data = malloc(cont->size);
                if (obj->name == NULL || obj->data == NULL) {
                    if (obj->name != NULL) free(obj->name);
                    if (obj->data != NULL) free(obj->data);
                    obj->name = NULL;
                    obj->data = NULL;
                    errno = ENOMEM;
                    break;
                }

                memcpy(obj->data, cont->data, cont->size);
            } else {
                obj->name = cont->name;
                obj->data = cont->data;
            }
            obj->size = cont->size;
            obj->prev = cont->prev;
            obj->next = cont->next;

            ret = true;
            break;
        }
    }

    unlock(tbl);
    return ret;
}

/**
 * (qlisttbl_t*)->remove(): Remove matched objects as given name.
 *
 * @param tbl   qlisttbl_t container pointer.
 * @param name  element name.
 *
 * @return a number of removed objects.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 */
static bool remove_(qlisttbl_t *tbl, const char *name)
{
    if (name == NULL) return false;

    lock(tbl);

    bool removed = false;
    qdlnobj_t *obj;
    for (obj = tbl->first; obj != NULL;) {
        if (!strcmp(obj->name, name)) { // found
            // copy next chain
            qdlnobj_t *prev = obj->prev;
            qdlnobj_t *next = obj->next;

            // adjust counter
            tbl->num--;
            removed = true;

            // remove list itself
            free(obj->name);
            free(obj->data);
            free(obj);

            // adjust chain links
            if (prev == NULL) tbl->first = next; // if the object is first one
            else prev->next = next;  // not the first one

            if (next == NULL) tbl->last = prev; // if the object is last one
            else next->prev = prev;  // not the first one

            // set next list
            obj = next;
        } else {
            // set next list
            obj = obj->next;
        }
    }

    unlock(tbl);

    if (removed == false) errno = ENOENT;
    return removed;
}

/**
 * (qlisttbl_t*)->setPutDirection(): Sets default adding direction(first or
 * last).
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param first     direction flag. false for adding at the end of this list by
 *                  default, true for adding at the beginning of this list by
 *                  default. (default is false)
 *
 * @return previous direction.
 *
 * @note
 *  The default direction is false which means adding new element at the end of
 *  list.
 */
static bool set_putdir(qlisttbl_t *tbl, bool first)
{
    bool prevdir = tbl->putdir;
    tbl->putdir = first;

    return prevdir;
}

/**
 * (qlisttbl_t*)->size(): Returns the number of elements in this list.
 *
 * @param tbl qlisttbl_t container pointer.
 *
 * @return the number of elements in this list.
 */
static size_t size(qlisttbl_t *tbl)
{
    return tbl->num;
}

/**
 * (qlisttbl_t*)->reverse(): Reverse the order of elements.
 *
 * @param tbl qlisttbl_t container pointer.
 *
 * @return true if successful otherwise returns false.
 */
static void reverse(qlisttbl_t *tbl)
{
    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj;) {
        qdlnobj_t *next = obj->next;
        obj->next = obj->prev;
        obj->prev = next;
        obj = next;
    }

    obj = tbl->first;
    tbl->first = tbl->last;
    tbl->last = obj;

    unlock(tbl);
}

/**
 * (qlisttbl_t*)->clear(): Removes all of the elements from this list.
 *
 * @param tbl qlisttbl_t container pointer.
 */
static void clear(qlisttbl_t *tbl)
{
    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj != NULL;) {
        qdlnobj_t *next = obj->next;
        free(obj->name);
        free(obj->data);
        free(obj);
        obj = next;
    }

    tbl->num = 0;
    tbl->first = NULL;
    tbl->last = NULL;
    unlock(tbl);
}

/**
 * (qlisttbl_t*)->parseStr(): Parse a string and replace variables in the string
 * to the data in this list.
 *
 * @param tbl   qlisttbl_t container pointer.
 * @param str   string value which may contain variables like ${...}
 *
 * @return malloced string if successful, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *
 * @code
 *  ${key_name}          - replace this with a matched value data in this list.
 *  ${!system_command}   - run external command and put it's output here.
 *  ${%PATH}             - get environment variable.
 * @endcode
 *
 * @code
 *  --[tbl Table]------------------------
 *  NAME = qLibc
 *  -------------------------------------
 *
 *  char *str = tbl->parseStr(tbl, "${NAME}, ${%HOME}, ${!date -u}");
 *  if(info != NULL) {
 *    printf("%s\n", str);
 *    free(str);
 *  }
 *
 *  [Output]
 *  qLibc, /home/qlibc, Wed Nov 24 00:30:58 UTC 2010
 * @endcode
 */
static char *parse_str(qlisttbl_t *tbl, const char *str)
{
    if (str == NULL) {
        errno = EINVAL;
        return NULL;
    }

    lock(tbl);

    bool loop;
    char *value = strdup(str);
    do {
        loop = false;

        // find ${
        char *s, *e;
        int openedbrakets;
        for (s = value; *s != '\0'; s++) {
            if (!(*s == _VAR && *(s+1) == _VAR_OPEN)) continue;

            // found ${, try to find }. s points $
            openedbrakets = 1; // braket open counter
            for (e = s + 2; *e != '\0'; e++) {
                if (*e == _VAR && *(e+1) == _VAR_OPEN) { // found internal ${
                    // e is always bigger than s, negative overflow never occure
                    s = e - 1;
                    break;
                } else if (*e == _VAR_OPEN) openedbrakets++;
                else if (*e == _VAR_CLOSE) openedbrakets--;
                else continue;

                if (openedbrakets == 0) break;
            }
            if (*e == '\0') break; // braket mismatch
            if (openedbrakets > 0) continue; // found internal ${

            // pick string between ${, }
            int varlen = e - s - 2; // length between ${ , }
            char *varstr = (char *)malloc(varlen + 3 + 1);
            if (varstr == NULL) continue;
            strncpy(varstr, s + 2, varlen);
            varstr[varlen] = '\0';

            // get the new string to replace
            char *newstr = NULL;
            switch (varstr[0]) {
                case _VAR_CMD : {
                    if ((newstr = qStrTrim(qSysCmd(varstr + 1))) == NULL) {
                        newstr = strdup("");
                    }
                    break;
                }
                case _VAR_ENV : {
                    newstr = strdup(qSysGetEnv(varstr + 1, ""));
                    break;
                }
                default : {
                    if ((newstr = get_str(tbl, varstr, true)) == NULL) {
                        s = e; // not found
                        continue;
                    }
                    break;
                }
            }

            // replace
            strncpy(varstr, s, varlen + 3); // ${str}
            varstr[varlen + 3] = '\0';

            s = qStrReplace("sn", value, varstr, newstr);
            free(newstr);
            free(varstr);
            free(value);
            value = s;

            loop = true;
            break;
        }
    } while (loop == true);

    unlock(tbl);

    return value;
}

/**
 * (qlisttbl_t*)->save(): Save qlisttbl_t as plain text format
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param filepath  save file path
 * @param sepchar   separator character between name and value. normally '=' is
 *                  used.
 * @param encode    flag for encoding data . false can be used if the all data
 *                  are string or integer type and has no new line. otherwise
 *                  true must be set.
 *
 * @return true if successful, otherwise returns false.
 */
static bool save(qlisttbl_t *tbl, const char *filepath, char sepchar,
                 bool encode)
{
    int fd;
    if ((fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, DEF_FILE_MODE)) < 0) {
        DEBUG("qlisttbl->save(): Can't open file %s", filepath);
        return false;
    }

    char *gmtstr = qTimeGetGmtStr(0);
    qIoPrintf(fd, 0, "# Generated by " _Q_PRGNAME " at %s.\n", gmtstr);
    qIoPrintf(fd, 0, "# %s\n", filepath);
    free(gmtstr);

    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj; obj = obj->next) {
        char *encval;
        if (encode == true) encval = qUrlEncode(obj->data, obj->size);
        else encval = obj->data;
        qIoPrintf(fd, 0, "%s%c%s\n", obj->name, sepchar, encval);
        if (encode == true) free(encval);
    }
    unlock(tbl);

    close(fd);
    return true;
}

/**
 * (qlisttbl_t*)->load(): Load and append entries from given filepath
 *
 * @param tbl       qlisttbl_t container pointer.
 * @param filepath  save file path
 * @param sepchar   separator character between name and value. normally '=' is
 *                  used
 * @param decode    flag for decoding data
 *
 * @return a number of loaded entries.
 */
static size_t load(qlisttbl_t *tbl, const char *filepath, char sepchar,
                   bool decode)
{
    qlisttbl_t *loaded;
    if ((loaded = qConfigParseFile(NULL, filepath, sepchar)) == NULL) {
        return false;
    }

    lock(tbl);
    size_t cnt;
    qdlnobj_t *obj;
    for (cnt = 0, obj = loaded->first; obj; obj = obj->next) {
        if (decode == true) obj->size = qUrlDecode(obj->data);
        put(tbl, obj->name, obj->data, obj->size, false);
        cnt++;
    }

    terminate(loaded);
    unlock(tbl);

    return cnt;
}

/**
 * (qlisttbl_t*)->debug(): Print out stored elements for debugging purpose.
 *
 * @param tbl qlisttbl_t container pointer.
 * @param out output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EIO : Invalid output stream.
 */
static bool debug(qlisttbl_t *tbl, FILE *out)
{
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj; obj = obj->next) {
        fprintf(out, "%s=" , obj->name);
        _q_humanOut(out, obj->data, obj->size, MAX_HUMANOUT);
        fprintf(out, " (%zu)\n" , obj->size);
    }
    unlock(tbl);

    return true;
}

/**
 * (qlisttbl_t*)->lock(): Enter critical section.
 *
 * @param tbl qlisttbl_t container pointer.
 *
 * @note
 *  From user side, normally locking operation is only needed when traverse all
 *  elements using (qlisttbl_t*)->get_next(). Most of other operations do
 *  necessary locking internally when it's compiled with --enable-threadsafe
 *  option.
 *
 * @note
 *  This operation will be activated only when --enable-threadsafe option is
 *  given at compile time. To find out whether it's compiled with threadsafe
 *  option, call qlibc_is_threadsafe().
 */
static void lock(qlisttbl_t *tbl)
{
    Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * (qlisttbl_t*)->unlock(): Leave critical section.
 *
 * @param tbl qlisttbl_t container pointer.
 *
 * @note
 *  This operation will be activated only when --enable-threadsafe option is
 *  given at compile time. To find out whether it's compiled with threadsafe
 *  option, call qlibc_is_threadsafe().
 */
static void unlock(qlisttbl_t *tbl)
{
    Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * (qlisttbl_t*)->terminate(): Free qlisttbl_t
 *
 * @param tbl qlisttbl_t container pointer.
 */
static void terminate(qlisttbl_t *tbl)
{
    clear(tbl);
    Q_MUTEX_DESTROY(tbl->qmutex);

    free(tbl);
}

#ifndef _DOXYGEN_SKIP

static bool _put(qlisttbl_t *tbl, const char *name, const void *data,
                 size_t size, bool unique, bool first)
{
    // check arguments
    if (name == NULL || data == NULL || size <= 0) {
        errno = EINVAL;
        return false;
    }

    // duplicate name. name can be NULL
    char *dup_name = strdup(name);
    if (dup_name == NULL) {
        errno = ENOMEM;
        return false;
    }

    // duplicate object
    void *dup_data = malloc(size);
    if (dup_data == NULL) {
        free(dup_name);
        errno = ENOMEM;
        return false;
    }
    memcpy(dup_data, data, size);

    // make new object list
    qdlnobj_t *obj = (qdlnobj_t *)malloc(sizeof(qdlnobj_t));
    if (obj == NULL) {
        free(dup_name);
        free(dup_data);
        errno = ENOMEM;
        return false;
    }
    obj->name = dup_name;
    obj->data = dup_data;
    obj->size = size;
    obj->prev = NULL;
    obj->next = NULL;

    lock(tbl);

    // if unique flag is set, remove same key
    if (unique == true) remove_(tbl, dup_name);

    // make link
    if (tbl->num == 0) {
        tbl->first = tbl->last = obj;
    } else {
        if (first == true) {
            obj->next = tbl->first;
            obj->next->prev = obj;
            tbl->first = obj;
        } else {
            obj->prev = tbl->last;
            tbl->last->next = obj;
            tbl->last = obj;
        }
    }

    tbl->num++;

    unlock(tbl);

    return true;
}

static void *_get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem,
                  bool backward, int (*cmp)(const char *s1, const char *s2))
{
    if (name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    lock(tbl);
    void *data = NULL;
    qdlnobj_t *obj;

    if (backward == false) obj = tbl->first;
    else obj = tbl->last;

    while (obj != NULL) {
        if (!cmp(obj->name, name)) {
            if (newmem == true) {
                data = malloc(obj->size);
                if (data == NULL) {
                    errno = ENOMEM;
                    return false;
                }
                memcpy(data, obj->data, obj->size);
            } else {
                data = obj->data;
            }
            if (size != NULL) *size = obj->size;

            break;
        }

        if (backward == false) obj = obj->next;
        else obj = obj->prev;
    }
    unlock(tbl);

    if (data == NULL) {
        errno = ENOENT;
    }

    return data;
}

#endif /* _DOXYGEN_SKIP */
