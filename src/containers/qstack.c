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
 * @file qstack.c Stack implementation.
 *
 * qstack_t container is a stack implementation. It represents a
 * last-in-first-out(LIFO). It extends container Q_LIST that allow a linked-list
 * to be treated as a stack.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *                       top     bottom
 *  DATA PUSH/POP <==> [ C ][ B ][ A ]
 *  (positive index)     0    1    2
 *  (negative index)    -3   -2   -1
 * @endcode
 *
 * @code
 *  // create stack
 *  qstack_t *stack = qstack();
 *
 *  // example: integer stack
 *  stack->push_int(stack, 1);
 *  stack->push_int(stack, 2);
 *  stack->push_int(stack, 3);
 *
 *  printf("pop_int(): %d\n", stack->pop_int(stack));
 *  printf("pop_int(): %d\n", stack->pop_int(stack));
 *  printf("pop_int(): %d\n", stack->pop_int(stack));
 *
 *  // example: string stack
 *  stack->push_str(stack, "A string");
 *  stack->push_str(stack, "B string");
 *  stack->push_str(stack, "C string");
 *
 *  char *str = stack->pop_str(stack);
 *  printf("pop_str(): %s\n", str);
 *  free(str);
 *  str = stack->pop_str(stack);
 *  printf("pop_str(): %s\n", str);
 *  free(str);
 *  str = stack->pop_str(stack);
 *  printf("pop_str(): %s\n", str);
 *  free(str);
 *
 *  // example: object stack
 *  stack->push(stack, "A object", sizeof("A object"));
 *  stack->push(stack, "B object", sizeof("B object"));
 *  stack->push(stack, "C object", sizeof("C object"));
 *
 *  void *obj = stack->pop(stack, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = stack->pop(stack, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = stack->pop(stack, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *
 *  // release
 *  stack->free(stack);
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
#include "qinternal.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP

static size_t set_size(qstack_t *stack, size_t max);

static bool push(qstack_t *stack, const void *data, size_t size);
static bool push_str(qstack_t *stack, const char *str);
static bool push_int(qstack_t *stack, int num);

static void *pop(qstack_t *stack, size_t *size);
static char *pop_str(qstack_t *stack);
static int pop_int(qstack_t *stack);
static void *pop_at(qstack_t *stack, int index, size_t *size);

static void *get(qstack_t *stack, size_t *size, bool newmem);
static char *get_str(qstack_t *stack);
static int get_int(qstack_t *stack);
static void *get_at(qstack_t *stack, int index, size_t *size, bool newmem);

static size_t size(qstack_t *stack);
static void clear(qstack_t *stack);
static bool debug(qstack_t *stack, FILE *out);
static void free_(qstack_t *stack);

#endif

/**
 * Create a new stack container
 *
 * @return a pointer of malloced qstack_t container, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @code
 *   qstack_t *stack = qstack();
 * @endcode
 */
qstack_t *qstack(void)
{
    qstack_t *stack = (qstack_t *)malloc(sizeof(qstack_t));
    if (stack == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset((void *)stack, 0, sizeof(qstack_t));
    stack->list = qlist();
    if (stack->list == NULL) {
        free(stack);
        return NULL;
    }

    // methods
    stack->set_size     = set_size;

    stack->push         = push;
    stack->push_str     = push_str;
    stack->push_int     = push_int;

    stack->pop          = pop;
    stack->pop_str      = pop_str;
    stack->pop_int      = pop_int;
    stack->pop_at       = pop_at;

    stack->get          = get;
    stack->get_str      = get_str;
    stack->get_int      = get_int;
    stack->get_at       = get_at;

    stack->size         = size;
    stack->clear        = clear;
    stack->debug        = debug;
    stack->free         = free_;

    return stack;
}

/**
 * (qstack_t*)->set_size(): Sets maximum number of elements allowed in this
 * stack.
 *
 * @param stack qstack container pointer.
 * @param max   maximum number of elements. 0 means no limit.
 *
 * @return previous maximum number.
 */
static size_t set_size(qstack_t *stack, size_t max)
{
    return stack->list->set_size(stack->list, max);
}

/**
 * (qstack_t*)->push(): Pushes an element onto the top of this stack.
 *
 * @param stack qstack container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Stack full. Only happens when this stack has set to have
 *                limited number of elements)
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push(qstack_t *stack, const void *data, size_t size)
{
    return stack->list->add_first(stack->list, data, size);
}

/**
 * (qstack_t*)->push_str(): Pushes a string onto the top of this stack.
 *
 * @param stack qstack container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Stack full. Only happens when this stack has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push_str(qstack_t *stack, const char *str)
{
    if (str == NULL) {
        errno = EINVAL;
        return false;
    }
    return stack->list->add_first(stack->list, str, strlen(str) + 1);
}

/**
 * (qstack_t*)->push_int(): Pushes a integer onto the top of this stack.
 *
 * @param stack qstack container pointer.
 * @param num   integer data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Stack full. Only happens when this stack has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push_int(qstack_t *stack, int num)
{
    return stack->list->add_first(stack->list, &num, sizeof(int));
}

/**
 * (qstack_t*)->pop(): Removes a element at the top of this stack and returns
 * that element.
 *
 * @param stack qstack container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 */
static void *pop(qstack_t *stack, size_t *size)
{
    return stack->list->pop_first(stack->list, size);
}

/**
 * (qstack_t*)->pop_str(): Removes a element at the top of this stack and
 * returns that element.
 *
 * @param stack qstack container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through push_str().
 */
static char *pop_str(qstack_t *stack)
{
    size_t strsize;
    char *str = stack->list->pop_first(stack->list, &strsize);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * (qstack_t*)->pop_int(): Removes a integer at the top of this stack and
 * returns that element.
 *
 * @param stack qstack container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through push_int().
 */
static int pop_int(qstack_t *stack)
{
    int num = 0;
    int *pnum = stack->list->pop_first(stack->list, NULL);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * (qstack_t*)->pop_at(): Returns and remove the element at the specified
 * position in this stack.
 *
 * @param stack qstack container pointer.
 * @param index index at which the specified element is to be inserted
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ERANGE    : Index out of range.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  Negative index can be used for addressing a element from the bottom in
 *  this stack. For example, index -1 will always pop a element which is pushed
 *  at very first time.
 */
static void *pop_at(qstack_t *stack, int index, size_t *size)
{
    return stack->list->pop_at(stack->list, index, size);
}

/**
 * (qstack_t*)->get(): Returns an element at the top of this stack without
 * removing it.
 *
 * @param stack     qstack container pointer.
 * @param size      if size is not NULL, element size will be stored.
 * @param newmem    whether or not to allocate memory for the element.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 */
static void *get(qstack_t *stack, size_t *size, bool newmem)
{
    return stack->list->get_first(stack->list, size, newmem);
}

/**
 * (qstack_t*)->get_str(): Returns an string at the top of this stack without
 * removing it.
 *
 * @param stack qstack container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through push_str().
 */
static char *get_str(qstack_t *stack)
{
    size_t strsize;
    char *str = stack->list->get_first(stack->list, &strsize, true);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * (qstack_t*)->get_int(): Returns an integer at the top of this stack without
 * removing it.
 *
 * @param stack qstack container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through push_int().
 */
static int get_int(qstack_t *stack)
{
    int num = 0;
    int *pnum = stack->list->get_first(stack->list, NULL, true);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * (qstack_t*)->get_at(): Returns an element at the specified position in this
 * stack without removing it.
 *
 * @param stack     qstack container pointer.
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
 * Negative index can be used for addressing a element from the bottom in this
 * stack. For example, index -1 will always get a element which is pushed at
 * very first time.
 */
static void *get_at(qstack_t *stack, int index, size_t *size, bool newmem)
{
    return stack->list->get_at(stack->list, index, size, newmem);
}

/**
 * (qstack_t*)->size(): Returns the number of elements in this stack.
 *
 * @param stack qstack container pointer.
 *
 * @return the number of elements in this stack.
 */
static size_t size(qstack_t *stack)
{
    return stack->list->size(stack->list);
}

/**
 * (qstack_t*)->clear(): Removes all of the elements from this stack.
 *
 * @param stack qstack container pointer.
 */
static void clear(qstack_t *stack)
{
    stack->list->clear(stack->list);
}

/**
 * (qstack_t*)->debug(): Print out stored elements for debugging purpose.
 *
 * @param stack     qstack container pointer.
 * @param out       output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 */
static bool debug(qstack_t *stack, FILE *out)
{
    return stack->list->debug(stack->list, out);
}

/**
 * (qstack_t*)->free(): Free qstack_t
 *
 * @param stack qstack container pointer.
 *
 * @return always returns true.
 */
static void free_(qstack_t *stack)
{
    stack->list->free(stack->list);
    free(stack);
}
