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
 * @file qlist.c Doubly Linked-list implementation.
 *
 * qlist_t container is a doubly Linked-List implementation.
 * qlist_t provides uniformly named methods to add, get, pop and remove an
 * element at the beginning and end of the list. These operations allow qlist_t
 * to be used as a stack, queue, or double-ended queue.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *  last~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
 *                                                          |
 *          +-----------+  doubly  +-----------+  doubly  +-|---------+
 *  first~~~|~>   0   <~|~~~~~~~~~~|~>   1   <~|~~~~~~~~~~|~>   N     |
 *          +-----|-----+  linked  +-----|-----+  linked  +-----|-----+
 *                |                      |                      |
 *          +-----v---------------+      |                +-----v-----+
 *          | DATA A              |      |                | DATA N    |
 *          +---------------------+      |                +-----------+
 *                 +---------------------v------------------+
 *                 | DATA B                                 |
 *                 +----------------------------------------+
 * @endcode
 *
 * @code
 *  // create a list.
 *  qlist_t *list = qlist();
 *
 *  // insert elements
 *  list->add_last(list, "e1", sizeof("e1"));
 *  list->add_last(list, "e2", sizeof("e2"));
 *  list->add_last(list, "e3", sizeof("e3"));
 *
 *  // get
 *  char *e1 = (char*)list->get_first(list, NULL, true));    // malloced
 *  char *e3  = (char*)list->get_at(list, -1, NULL, false)); // no malloc
 *  (...omit...)
 *  free(e1);
 *
 *  // pop (get and remove)
 *  char *e2 = (char*)list->pop_at(list, 1, NULL)); // get malloced copy
 *  (...omit...)
 *  free(e2);
 *
 *  // debug output
 *  list->debug(list, stdout, true);
 *
 *  // travesal
 *  qdlobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  list->lock(list);
 *  while (list->get_next(list, &obj, false) == true) {
 *    printf("DATA=%s, SIZE=%zu\n", (char*)obj.data, obj.size);
 *  }
 *  list->unlock(list);
 *
 *  // free object
 *  list->free(list);
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
#include <errno.h>
#include "qlibc.h"
#include "qinternal.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP
static bool add_first(qlist_t *list, const void *data, size_t size);
static bool add_last(qlist_t *list, const void *data, size_t size);
static bool add_at(qlist_t *list, int index, const void *data, size_t size);

static void *get_first(qlist_t *list, size_t *size, bool newmem);
static void *get_last(qlist_t *list, size_t *size, bool newmem);
static void *get_at(qlist_t *list, int index, size_t *size, bool newmem);
static bool get_next(qlist_t *list, qdlobj_t *obj, bool newmem);

static void *pop_first(qlist_t *list, size_t *size);
static void *pop_last(qlist_t *list, size_t *size);
static void *pop_at(qlist_t *list, int index, size_t *size);

static bool remove_first(qlist_t *list);
static bool remove_last(qlist_t *list);
static bool remove_at(qlist_t *list, int index);

static size_t set_size(qlist_t *list, size_t max);
static size_t size(qlist_t *list);
static size_t datasize(qlist_t *list);
static void reverse(qlist_t *list);
static void clear(qlist_t *list);

static void *to_array(qlist_t *list, size_t *size);
static char *to_string(qlist_t *list);
static bool qlist_debug(qlist_t *list, FILE *out);

static void lock(qlist_t *list);
static void unlock(qlist_t *list);

static void free_(qlist_t *list);

/* internal functions */
static void *_get_at(qlist_t *list,
                     int index, size_t *size, bool newmem, bool remove);
static qdlobj_t *_get_obj(qlist_t *list, int index);
static bool _remove_obj(qlist_t *list, qdlobj_t *obj);
#endif

/**
 * Create new qlist_t linked-list container
 *
 * @return a pointer of malloced qlist_t container, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOMEM : Memory allocation failure.
 *
 * @code
 *  qlist_t *list = qlist();
 * @endcode
 */
qlist_t *qlist(void)
{
    qlist_t *list = (qlist_t *)malloc(sizeof(qlist_t));
    if (list == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    /* initialize container */
    memset((void *)list, 0, sizeof(qlist_t));

    /* member methods */
    list->add_first     = add_first;
    list->add_last      = add_last;
    list->add_at        = add_at;

    list->get_first     = get_first;
    list->get_last      = get_last;
    list->get_at        = get_at;
    list->get_next      = get_next;

    list->pop_first     = pop_first;
    list->pop_last      = pop_last;
    list->pop_at        = pop_at;

    list->remove_first  = remove_first;
    list->remove_last   = remove_last;
    list->remove_at     = remove_at;

    list->reverse       = reverse;
    list->clear         = clear;

    list->set_size      = set_size;
    list->size          = size;
    list->datasize      = datasize;

    list->to_array      = to_array;
    list->to_string     = to_string;
    list->debug         = qlist_debug;

    list->lock          = lock;
    list->unlock        = unlock;

    list->free          = free_;

    // initialize recrusive mutex
    Q_MUTEX_INIT(list->qmutex, true);

    return list;
}

/**
 * (qlist_t*)->add_first(): Inserts a element at the beginning of this list.
 *
 * @param list  qlist_t container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS : List full. Only happens when this list has set to have limited
 *              number of elements.
 *  - EINVAL  : Invalid argument.
 *  - ENOMEM  : Memory allocation failure.
 *
 * @code
 *  // create a sample object.
 *  struct my_obj obj;
 *
 *  // create a list and add the sample object.
 *  qlist_t *list = qlist();
 *  list->add_first(list, &obj, sizeof(struct my_obj));
 * @endcode
 */
static bool add_first(qlist_t *list, const void *data, size_t size)
{
    return add_at(list, 0, data, size);
}

/**
 * (qlist_t*)->add_last(): Appends a element to the end of this list.
 *
 * @param list  qlist_t container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS : List full. Only happens when this list has set to have limited
 *              number of elements.
 *  - EINVAL  : Invalid argument.
 *  - ENOMEM  : Memory allocation failure.
 */
static bool add_last(qlist_t *list, const void *data, size_t size)
{
    return add_at(list, -1, data, size);
}

/**
 * (qlist_t*)->add_at(): Inserts a element at the specified position in this
 * list.
 *
 * @param list   qlist_t container pointer.
 * @param index  index at which the specified element is to be inserted.
 * @param data   a pointer which points data memory.
 * @param size   size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS : List full. Only happens when this list has set to have limited
 *              number of elements.
 *  - ERANGE  : Index out of range.
 *  - EINVAL  : Invalid argument.
 *  - ENOMEM  : Memory allocation failure.
 *
 * @code
 *                     first           last      new
 *  Linked-list        [ A ]<=>[ B ]<=>[ C ]?==?[   ]
 *  (positive index)     0       1       2        3
 *  (negative index)    -3      -2      -1
 * @endcode
 *
 * @code
 *  qlist_t *list = qlist();
 *  list->add_at(list, 0, &obj, sizeof(obj));  // same as add_first().
 *  list->add_at(list, -1, &obj, sizeof(obj)); // same as add_last().
 * @endcode
 *
 * @note
 *  Index starts from 0.
 */
static bool add_at(qlist_t *list,
                   int index, const void *data, size_t size)
{
    // check arguments
    if (data == NULL || size <= 0) {
        errno = EINVAL;
        return false;
    }

    lock(list);

    // check maximum number of allowed elements if set
    if (list->max > 0 && list->num >= list->max) {
        errno = ENOBUFS;
        unlock(list);
        return false;
    }

    // adjust index
    if (index < 0) index = (list->num + index) + 1;  // -1 is same as add_last()
    if (index < 0 || index > list->num) {
        // out of bound
        unlock(list);
        errno = ERANGE;
        return false;
    }

    // duplicate object
    void *dup_data = malloc(size);
    if (dup_data == NULL) {
        unlock(list);
        errno = ENOMEM;
        return false;
    }
    memcpy(dup_data, data, size);

    // make new object list
    qdlobj_t *obj = (qdlobj_t *)malloc(sizeof(qdlobj_t));
    if (obj == NULL) {
        free(dup_data);
        unlock(list);
        errno = ENOMEM;
        return false;
    }
    obj->data = dup_data;
    obj->size = size;
    obj->prev = NULL;
    obj->next = NULL;

    // make link
    if (index == 0) {
        // add at first
        obj->next = list->first;
        if (obj->next != NULL) obj->next->prev = obj;
        list->first = obj;
        if (list->last == NULL) list->last = obj;
    } else if (index == list->num) {
        // add after last
        obj->prev = list->last;
        if (obj->prev != NULL) obj->prev->next = obj;
        list->last = obj;
        if (list->first == NULL) list->first = obj;
    } else {
        // add at the middle of list
        qdlobj_t *tgt = _get_obj(list, index);
        if (tgt == NULL) {
            // should not be happened.
            free(dup_data);
            free(obj);
            unlock(list);
            errno = EAGAIN;
            return false;
        }

        // insert obj
        tgt->prev->next = obj;
        obj->prev = tgt->prev;
        obj->next = tgt;
        tgt->prev = obj;
    }

    list->datasum += size;
    list->num++;

    unlock(list);

    return true;
}

/**
 * (qlist_t*)->get_first(): Returns the first element in this list.
 *
 * @param list    qlist_t container pointer.
 * @param size    if size is not NULL, element size will be stored.
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : List is empty.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  size_t size;
 *  void *data = list->get_first(list, &size, true);
 *  if (data != NULL) {
 *    (...omit...)
 *    free(data);
 *  }
 * @endcode
 */
static void *get_first(qlist_t *list, size_t *size, bool newmem)
{
    return get_at(list, 0, size, newmem);
}

/**
 * (qlist_t*)->get_last(): Returns the last element in this list.
 *
 * @param list    qlist_t container pointer.
 * @param size    if size is not NULL, element size will be stored.
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *        ENOENT : List is empty.
 *        ENOMEM : Memory allocation failure.
 */
static void *get_last(qlist_t *list, size_t *size, bool newmem)
{
    return get_at(list, -1, size, newmem);
}

/**
 * (qlist_t*)->get_at(): Returns the element at the specified position in this
 * list.
 *
 * @param list    qlist_t container pointer.
 * @param index   index at which the specified element is to be inserted
 * @param size    if size is not NULL, element size will be stored.
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno
 * @retval errno will be set in error condition.
 *  -ERANGE : Index out of range.
 *  -ENOMEM : Memory allocation failure.
 *
 * @code
 *                     first           last
 *  Linked-list        [ A ]<=>[ B ]<=>[ C ]
 *  (positive index)     0       1       2
 *  (negative index)    -3      -2      -1
 * @endcode
 *
 * @note
 *  Negative index can be used for addressing a element from the end in this
 *  stack. For example, index -1 is same as get_last() and index 0 is same as
 *  get_first();
 */
static void *get_at(qlist_t *list, int index, size_t *size, bool newmem)
{
    return _get_at(list, index, size, newmem, false);
}

/**
 * (qlist_t*)->get_next(): Get next element in this list.
 *
 * @param list    qlist_t container pointer.
 * @param obj     found data will be stored in this structure
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return true if found otherwise returns false
 * @retval errno will be set in error condition.
 *  -ENOENT : No next element.
 *  -ENOMEM : Memory allocation failure.
 *
 * @note
 *  obj should be initialized with 0 by using memset() before first call.
 *  If newmem flag is true, user should de-allocate obj.name and obj.data
 *  resources.
 *
 * @code
 *  qlist_t *list = qlist();
 *  (...add data into list...)
 *
 *  qdlobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  list->lock(list);   // can be omitted in single thread model.
 *  while (list->get_next(list, &obj, false) == true) {
 *   printf("DATA=%s, SIZE=%zu\n", (char*)obj.data, obj.size);
 *  }
 *  list->unlock(list); // release lock.
 * @endcode
 */
static bool get_next(qlist_t *list, qdlobj_t *obj, bool newmem)
{
    if (obj == NULL) return NULL;

    lock(list);

    qdlobj_t *cont = NULL;
    if (obj->size == 0) cont = list->first;
    else cont = obj->next;

    if (cont == NULL) {
        errno = ENOENT;
        unlock(list);
        return false;
    }

    bool ret = false;
    while (cont != NULL) {
        if (newmem == true) {
            obj->data = malloc(cont->size);
            if (obj->data == NULL) break;

            memcpy(obj->data, cont->data, cont->size);
        } else {
            obj->data = cont->data;
        }
        obj->size = cont->size;
        obj->prev = cont->prev;
        obj->next = cont->next;

        ret = true;
        break;
    }

    unlock(list);
    return ret;
}

/**
 * (qlist_t*)->pop_first(): Returns and remove the first element in this list.
 *
 * @param list  qlist_t container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 */
static void *pop_first(qlist_t *list, size_t *size)
{
    return pop_at(list, 0, size);
}


/**
 * (qlist_t*)->get_last(): Returns and remove the last element in this list.
 *
 * @param list  qlist_t container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 */
static void *pop_last(qlist_t *list, size_t *size)
{
    return pop_at(list, -1, size);
}

/**
 * (qlist_t*)->pop_at(): Returns and remove the element at the specified
 * position in this list.
 *
 * @param list   qlist_t container pointer.
 * @param index  index at which the specified element is to be inserted
 * @param size   if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ERANGE : Index out of range.
 *  -ENOMEM : Memory allocation failure.
 *
 * @code
 *                     first           last
 *  Linked-list        [ A ]<=>[ B ]<=>[ C ]
 *  (positive index)     0       1       2
 *  (negative index)    -3      -2      -1
 * @endcode
 *
 * @note
 *  Negative index can be used for addressing a element from the end in this
 *  stack. For example, index -1 is same as pop_last() and index 0 is same as
 *  pop_first();
 */
static void *pop_at(qlist_t *list, int index, size_t *size)
{
    return _get_at(list, index, size, true, true);
}

/**
 * (qlist_t*)->remove_first(): Removes the first element in this list.
 *
 * @param list  qlist_t container pointer.
 *
 * @return a number of removed objects.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 */
static bool remove_first(qlist_t *list)
{
    return remove_at(list, 0);
}

/**
 * (qlist_t*)->remove_last(): Removes the last element in this list.
 *
 * @param list  qlist_t container pointer.
 *
 * @return a number of removed objects.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 */
static bool remove_last(qlist_t *list)
{
    return remove_at(list, -1);
}

/**
 * (qlist_t*)->remove_at(): Removes the element at the specified position in
 * this list.
 *
 * @param list   qlist_t container pointer.
 * @param index  index at which the specified element is to be removed.
 *
 * @return a number of removed objects.
 * @retval errno will be set in error condition.
 *  -ERANGE : Index out of range.
 */
static bool remove_at(qlist_t *list, int index)
{
    lock(list);

    // get object pointer
    qdlobj_t *obj = _get_obj(list, index);
    if (obj == NULL) {
        unlock(list);
        return false;
    }

    bool ret = _remove_obj(list, obj);

    unlock(list);

    return ret;
}

/**
 * (qlist_t*)->set_size(): Sets maximum number of elements allowed in this
 * list.
 *
 * @param list  qlist_t container pointer.
 * @param max   maximum number of elements. 0 means no limit.
 *
 * @return previous maximum number.
 *
 * @note
 *  The default maximum number of elements is unlimited.
 */
static size_t set_size(qlist_t *list, size_t max)
{
    lock(list);
    size_t old = list->max;
    list->max = max;
    unlock(list);
    return old;
}

/**
 * (qlist_t*)->size(): Returns the number of elements in this list.
 *
 * @param list  qlist_t container pointer.
 *
 * @return the number of elements in this list.
 */
static size_t size(qlist_t *list)
{
    return list->num;
}

/**
 * (qlist_t*)->size(): Returns the sum of total element size.
 *
 * @param list  qlist_t container pointer.
 *
 * @return the sum of total element size.
 */
static size_t datasize(qlist_t *list)
{
    return list->datasum;
}

/**
 * (qlist_t*)->reverse(): Reverse the order of elements.
 *
 * @param list  qlist_t container pointer.
 */
static void reverse(qlist_t *list)
{
    lock(list);
    qdlobj_t *obj;
    for (obj = list->first; obj;) {
        qdlobj_t *next = obj->next;
        obj->next = obj->prev;
        obj->prev = next;
        obj = next;
    }

    obj = list->first;
    list->first = list->last;
    list->last = obj;

    unlock(list);
}

/**
 * (qlist_t*)->clear(): Removes all of the elements from this list.
 *
 * @param list  qlist_t container pointer.
 */
static void clear(qlist_t *list)
{
    lock(list);
    qdlobj_t *obj;
    for (obj = list->first; obj;) {
        qdlobj_t *next = obj->next;
        free(obj->data);
        free(obj);
        obj = next;
    }

    list->num = 0;
    list->datasum = 0;
    list->first = NULL;
    list->last = NULL;
    unlock(list);
}

/**
 * (qlist_t*)->to_array(): Returns the serialized chunk containing all the
 * elements in this list.
 *
 * @param list  qlist_t container pointer.
 * @param size  if size is not NULL, chunk size will be stored.
 *
 * @return a malloced pointer,
 *  otherwise(if there is no data to merge) returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 */
static void *to_array(qlist_t *list, size_t *size)
{
    if (list->num <= 0) {
        if (size != NULL) *size = 0;
        errno = ENOENT;
        return NULL;
    }

    lock(list);

    void *chunk = malloc(list->datasum);
    if (chunk == NULL) {
        unlock(list);
        errno = ENOMEM;
        return NULL;
    }
    void *dp = chunk;

    qdlobj_t *obj;
    for (obj = list->first; obj; obj = obj->next) {
        memcpy(dp, obj->data, obj->size);
        dp += obj->size;
    }
    unlock(list);

    if (size != NULL) *size = list->datasum;
    return chunk;
}

/**
 * (qlist_t*)->to_string(): Returns a string representation of this list,
 * containing string representation of each element.
 *
 * @param list  qlist_t container pointer.
 *
 * @return a malloced pointer,
 *  otherwise(if there is no data to merge) returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 *
 * @note
 *  Return string is always terminated by '\0'.
 */
static char *to_string(qlist_t *list)
{
    if (list->num <= 0) {
        errno = ENOENT;
        return NULL;
    }

    lock(list);

    void *chunk = malloc(list->datasum + 1);
    if (chunk == NULL) {
        unlock(list);
        errno = ENOMEM;
        return NULL;
    }
    void *dp = chunk;

    qdlobj_t *obj;
    for (obj = list->first; obj; obj = obj->next) {
        size_t size = obj->size;
        // do not copy tailing '\0'
        if (*(char *)(obj->data + (size - 1)) == '\0') size -= 1;
        memcpy(dp, obj->data, size);
        dp += size;
    }
    *((char *)dp) = '\0';
    unlock(list);

    return (char *)chunk;
}

/**
 * (qlist_t*)->debug(): Prints out stored elements for debugging purpose.
 *
 * @param list  qlist_t container pointer.
 * @param out   output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  -EIO  : Invalid output stream.
 */
static bool qlist_debug(qlist_t *list, FILE *out)
{
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    lock(list);
    qdlobj_t *obj;
    int i;
    for (i = 0, obj = list->first; obj; obj = obj->next, i++) {
        fprintf(out, "%d=" , i);
        _q_humanOut(out, obj->data, obj->size, MAX_HUMANOUT);
        fprintf(out, " (%zu)\n" , obj->size);
    }
    unlock(list);

    return true;
}

/**
 * (qlist_t*)->lock(): Enters critical section.
 *
 * @param list  qlist_t container pointer.
 *
 * @note
 *  From user side, normally locking operation is only needed when traverse all
 *  elements using (qlist_t*)->get_next(). Most of other operations do
 *  necessary locking internally when it's compiled with --enable-threadsafe
 *  option.
 *
 * @note
 *  This operation will be activated only when --enable-threadsafe option is
 *  given at compile time. To find out whether it's compiled with threadsafe
 *  option, call qlibc_is_threadsafe().
 */
static void lock(qlist_t *list)
{
    Q_MUTEX_ENTER(list->qmutex);
}

/**
 * (qlist_t*)->unlock(): Leaves critical section.
 *
 * @param list  qlist_t container pointer.
 *
 * @note
 *  This operation will be activated only when --enable-threadsafe option is
 *  given at compile time. To find out whether it's compiled with threadsafe
 *  option, call qlibc_is_threadsafe().
 */
static void unlock(qlist_t *list)
{
    Q_MUTEX_LEAVE(list->qmutex);
}

/**
 * (qlist_t*)->free(): Free qlist_t.
 *
 * @param list  qlist_t container pointer.
 */
static void free_(qlist_t *list)
{
    clear(list);
    Q_MUTEX_DESTROY(list->qmutex);

    free(list);
}

#ifndef _DOXYGEN_SKIP

static void *_get_at(qlist_t *list,
                     int index, size_t *size, bool newmem, bool remove)
{
    lock(list);

    // get object pointer
    qdlobj_t *obj = _get_obj(list, index);
    if (obj == NULL) {
        unlock(list);
        return false;
    }

    // copy data
    void *data;
    if (newmem == true) {
        data = malloc(obj->size);
        if (data == NULL) {
            unlock(list);
            errno = ENOMEM;
            return false;
        }
        memcpy(data, obj->data, obj->size);
    } else {
        data = obj->data;
    }
    if (size != NULL) *size = obj->size;

    // remove if necessary
    if (remove == true) {
        if (_remove_obj(list, obj) == false) {
            if (newmem == true) free(data);
            data = NULL;
        }
    }

    unlock(list);

    return data;
}

static qdlobj_t *_get_obj(qlist_t *list, int index)
{
    // index adjustment
    if (index < 0) index = list->num + index;
    if (index >= list->num) {
        errno = ERANGE;
        return NULL;
    }

    // detect faster scan direction
    bool backward;
    qdlobj_t *obj;
    int listidx;
    if (index < list->num / 2) {
        backward = false;
        obj = list->first;
        listidx = 0;
    } else {
        backward = true;
        obj = list->last;
        listidx = list->num - 1;
    }

    // find object
    while (obj != NULL) {
        if (listidx == index) return obj;

        if (backward == false) {
            obj = obj->next;
            listidx++;
        } else {
            obj = obj->prev;
            listidx--;
        }
    }

    // never reach here
    errno = ENOENT;
    return NULL;
}

static bool _remove_obj(qlist_t *list, qdlobj_t *obj)
{
    if (obj == NULL) return false;

    // chain prev and next elements
    if (obj->prev == NULL) list->first = obj->next;
    else obj->prev->next = obj->next;
    if (obj->next == NULL) list->last = obj->prev;
    else obj->next->prev = obj->prev;

    // adjust counter
    list->datasum -= obj->size;
    list->num--;

    // release obj
    free(obj->data);
    free(obj);

    return true;
}

#endif /* _DOXYGEN_SKIP */
