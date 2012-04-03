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
 * @file qqueue.c Queue implementation.
 *
 * qqueue_t container is a queue implementation. It represents a
 * first-in-first-out(FIFO). It extends container Q_LIST that allow a
 * linked-list to be treated as a queue.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *                      top     bottom
 *  DATA POP    <==    [ A ][ B ][ C ]  <== DATA PUSH
 *  (positive index)     0    1    2
 *  (negative index)    -3   -2   -1
 * @endcode
 *
 * @code
 *  // create queue
 *  qqueue_t *queue = qQueue();
 *
 *  // example: integer queue
 *  queue->push_int(queue, 1);
 *  queue->push_int(queue, 2);
 *  queue->push_int(queue, 3);
 *
 *  printf("pop_int(): %d\n", queue->pop_int(queue));
 *  printf("pop_int(): %d\n", queue->pop_int(queue));
 *  printf("pop_int(): %d\n", queue->pop_int(queue));
 *
 *  // example: string queue
 *  queue->push_str(queue, "A string");
 *  queue->push_str(queue, "B string");
 *  queue->push_str(queue, "C string");
 *
 *  char *str = queue->pop_str(queue);
 *  printf("pop_str(): %s\n", str);
 *  free(str);
 *  str = queue->pop_str(queue);
 *  printf("pop_str(): %s\n", str);
 *  free(str);
 *  str = queue->pop_str(queue);
 *  printf("pop_str(): %s\n", str);
 *  free(str);
 *
 *  // example: object queue
 *  queue->push(queue, "A object", sizeof("A object"));
 *  queue->push(queue, "B object", sizeof("B object"));
 *  queue->push(queue, "C object", sizeof("C object"));
 *
 *  void *obj = queue->pop(queue, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = queue->pop(queue, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = queue->pop(queue, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *
 *  // release
 *  queue->free(queue);
 *
 *  [Output]
 *  pop_int(): 3
 *  pop_int(): 2
 *  pop_int(): 1
 *  pop_str(): C string
 *  pop_str(): B string
 *  pop_str(): A string
 *  pop(): C object
 *  pop(): B object
 *  pop(): A object
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
#include "qInternal.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP

static size_t set_size(qqueue_t *queue, size_t max);

static bool push(qqueue_t *queue, const void *data, size_t size);
static bool push_str(qqueue_t *queue, const char *str);
static bool push_int(qqueue_t *queue, int num);

static void *pop(qqueue_t *queue, size_t *size);
static char *pop_str(qqueue_t *queue);
static int  pop_int(qqueue_t *queue);
static void *pop_at(qqueue_t *queue, int index, size_t *size);

static void *get(qqueue_t *queue, size_t *size, bool newmem);
static char *get_str(qqueue_t *queue);
static int get_int(qqueue_t *queue);
static void *get_at(qqueue_t *queue, int index, size_t *size, bool newmem);

static size_t size(qqueue_t *queue);
static void clear(qqueue_t *queue);
static bool debug(qqueue_t *queue, FILE *out);
static void terminate(qqueue_t *queue);

#endif

/**
 * Create new queue container
 *
 * @return a pointer of malloced qqueue container, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @code
 *   qqueue_t *queue = qQueue();
 * @endcode
 */
qqueue_t *qqueue(void)
{
    qqueue_t *queue = (qqueue_t *)malloc(sizeof(qqueue_t));
    if (queue == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset((void *)queue, 0, sizeof(qqueue_t));
    queue->list = qlist();
    if (queue->list == NULL) {
        free(queue);
        return NULL;
    }

    // methods
    queue->set_size     = set_size;

    queue->push         = push;
    queue->push_str     = push_str;
    queue->push_int     = push_int;

    queue->pop          = pop;
    queue->pop_str      = pop_str;
    queue->pop_int      = pop_int;
    queue->pop_at       = pop_at;

    queue->get          = get;
    queue->get_str      = get_str;
    queue->get_int      = get_int;
    queue->get_at       = get_at;

    queue->size         = size;
    queue->clear        = clear;
    queue->debug        = debug;
    queue->terminate    = terminate;

    return queue;
}

/**
 * (qqueue_t*)->set_size(): Sets maximum number of elements allowed in this
 * queue.
 *
 * @param queue qqueue container pointer.
 * @param max   maximum number of elements. 0 means no limit.
 *
 * @return previous maximum number.
 */
static size_t set_size(qqueue_t *queue, size_t max)
{
    return queue->list->set_size(queue->list, max);
}

/**
 * (qqueue_t*)->push(): Pushes an element onto the top of this queue.
 *
 * @param queue qqueue container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Queue full. Only happens when this queue has set to have
 *                limited number of elements)
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push(qqueue_t *queue, const void *data, size_t size)
{
    return queue->list->add_last(queue->list, data, size);
}

/**
 * (qqueue_t*)->push_str(): Pushes a string onto the top of this queue.
 *
 * @param queue qqueue container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Queue full. Only happens when this queue has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push_str(qqueue_t *queue, const char *str)
{
    if (str == NULL) {
        errno = EINVAL;
        return false;
    }
    return queue->list->add_last(queue->list, str, strlen(str) + 1);
}

/**
 * (qqueue_t*)->push_int(): Pushes a integer onto the top of this queue.
 *
 * @param queue qqueue container pointer.
 * @param num   integer data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Queue full. Only happens when this queue has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push_int(qqueue_t *queue, int num)
{
    return queue->list->add_last(queue->list, &num, sizeof(int));
}

/**
 * (qqueue_t*)->pop(): Removes a element at the top of this queue and returns
 * that element.
 *
 * @param queue qqueue container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 */
static void *pop(qqueue_t *queue, size_t *size)
{
    return queue->list->pop_first(queue->list, size);
}

/**
 * (qqueue_t*)->pop_str(): Removes a element at the top of this queue and
 * returns that element.
 *
 * @param queue qqueue container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through push_str().
 */
static char *pop_str(qqueue_t *queue)
{
    size_t strsize;
    char *str = queue->list->pop_first(queue->list, &strsize);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * (qqueue_t*)->pop_int(): Removes a integer at the top of this queue and
 * returns that element.
 *
 * @param queue qqueue container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through push_int().
 */
static int pop_int(qqueue_t *queue)
{
    int num = 0;
    int *pnum = queue->list->pop_first(queue->list, NULL);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * (qqueue_t*)->pop_at(): Returns and remove the element at the specified
 * position in this queue.
 *
 * @param queue qqueue container pointer.
 * @param index index at which the specified element is to be inserted
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ERANGE    : Index out of range.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  Negative index can be used for addressing a element from the bottom in this
 *  queue. For example, index -1 will always pop a element which is pushed at
 *  very last time.
 */
static void *pop_at(qqueue_t *queue, int index, size_t *size)
{
    return queue->list->pop_at(queue->list, index, size);
}

/**
 * (qqueue_t*)->get(): Returns an element at the top of this queue without
 * removing it.
 *
 * @param queue     qqueue container pointer.
 * @param size      if size is not NULL, element size will be stored.
 * @param newmem    whether or not to allocate memory for the element.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 */
static void *get(qqueue_t *queue, size_t *size, bool newmem)
{
    return queue->list->get_first(queue->list, size, newmem);
}

/**
 * (qqueue_t*)->get_str(): Returns an string at the top of this queue without
 * removing it.
 *
 * @param queue qqueue container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through push_str().
 */
static char *get_str(qqueue_t *queue)
{
    size_t strsize;
    char *str = queue->list->get_first(queue->list, &strsize, true);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * (qqueue_t*)->get_int(): Returns an integer at the top of this queue without
 * removing it.
 *
 * @param queue qqueue container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  The integer element should be pushed through push_int().
 */
static int get_int(qqueue_t *queue)
{
    int num = 0;
    int *pnum = queue->list->get_first(queue->list, NULL, true);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * (qqueue_t*)->get_at(): Returns an element at the specified position in this
 * queue without removing it.
 *
 * @param queue     qqueue container pointer.
 * @param index     index at which the specified element is to be inserted
 * @param size      if size is not NULL, element size will be stored.
 * @param newmem    whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ERANGE    : Index out of range.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  Negative index can be used for addressing a element from the bottom in this
 *  queue. For example, index -1 will always get a element which is pushed at
 *  very last time.
 */
static void *get_at(qqueue_t *queue, int index, size_t *size, bool newmem)
{
    return queue->list->get_at(queue->list, index, size, newmem);
}

/**
 * (qqueue_t*)->size(): Returns the number of elements in this queue.
 *
 * @param queue qqueue container pointer.
 *
 * @return the number of elements in this queue.
 */
static size_t size(qqueue_t *queue)
{
    return queue->list->size(queue->list);
}

/**
 * (qqueue_t*)->clear(): Removes all of the elements from this queue.
 *
 * @param queue qqueue container pointer.
 */
static void clear(qqueue_t *queue)
{
    queue->list->clear(queue->list);
}

/**
 * (qqueue_t*)->debug(): Print out stored elements for debugging purpose.
 *
 * @param queue     qqueue container pointer.
 * @param out       output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 */
static bool debug(qqueue_t *queue, FILE *out)
{
    return queue->list->debug(queue->list, out);
}

/**
 * (qqueue_t*)->terminate(): Free qqueue_t
 *
 * @param queue qqueue container pointer.
 *
 * @return always returns true.
 */
static void terminate(qqueue_t *queue)
{
    queue->list->terminate(queue->list);
    free(queue);
}
