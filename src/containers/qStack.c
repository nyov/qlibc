/*
 * qLibc - http://www.qdecoder.org
 *
 * Copyright 2010 qDecoder Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY QDECODER PROJECT ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE QDECODER PROJECT BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

/**
 * @file qStack.c Stack implementation.
 *
 * Q_STACK container is a stack implementation. It represents a last-in-first-out(LIFO).
 * It extends container Q_LIST that allow a linked-list to be treated as a stack.
 *
 * @code
 *   [Conceptional Data Structure Diagram]
 *
 *                       top     bottom
 *   DATA PUSH/POP <==> [ C ][ B ][ A ]
 *   (positive index)     0    1    2
 *   (negative index)    -3   -2   -1
 * @endcode
 *
 * @code
 *   // create stack
 *   Q_STACK *stack = qStack();
 *
 *   // example: integer stack
 *   stack->pushInt(stack, 1);
 *   stack->pushInt(stack, 2);
 *   stack->pushInt(stack, 3);
 *
 *   printf("popInt(): %d\n", stack->popInt(stack));
 *   printf("popInt(): %d\n", stack->popInt(stack));
 *   printf("popInt(): %d\n", stack->popInt(stack));
 *
 *   // example: string stack
 *   stack->pushStr(stack, "A string");
 *   stack->pushStr(stack, "B string");
 *   stack->pushStr(stack, "C string");
 *
 *   char *str = stack->popStr(stack);
 *   printf("popStr(): %s\n", str);
 *   free(str);
 *   str = stack->popStr(stack);
 *   printf("popStr(): %s\n", str);
 *   free(str);
 *   str = stack->popStr(stack);
 *   printf("popStr(): %s\n", str);
 *   free(str);
 *
 *   // example: object stack
 *   stack->push(stack, "A object", sizeof("A object"));
 *   stack->push(stack, "B object", sizeof("B object"));
 *   stack->push(stack, "C object", sizeof("C object"));
 *
 *   void *obj = stack->pop(stack, NULL);
 *   printf("pop(): %s\n", (char*)obj);
 *   free(obj);
 *   str = stack->pop(stack, NULL);
 *   printf("pop(): %s\n", (char*)obj);
 *   free(obj);
 *   str = stack->pop(stack, NULL);
 *   printf("pop(): %s\n", (char*)obj);
 *   free(obj);
 *
 *   // release
 *   stack->free(stack);
 *
 * [Output]
 *   popInt(): 3
 *   popInt(): 2
 *   popInt(): 1
 *   popStr(): C string
 *   popStr(): B string
 *   popStr(): A string
 *   pop(): C object
 *   pop(): B object
 *   pop(): A object
 * @endcode
 *
 * @note
 * Use "--enable-threadsafe" configure script option to use under multi-threaded environments.
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

static size_t	_setSize(Q_STACK *stack, size_t max);

static bool	_push(Q_STACK *stack, const void *data, size_t size);
static bool	_pushStr(Q_STACK *stack, const char *str);
static bool	_pushInt(Q_STACK *stack, int num);

static void*	_pop(Q_STACK *stack, size_t *size);
static char*	_popStr(Q_STACK *stack);
static int	_popInt(Q_STACK *stack);
static void*	_popAt(Q_STACK *stack, int index, size_t *size);

static void*	_get(Q_STACK *stack, size_t *size, bool newmem);
static char*	_getStr(Q_STACK *stack);
static int	_getInt(Q_STACK *stack);
static void*	_getAt(Q_STACK *stack, int index, size_t *size, bool newmem);

static size_t	_size(Q_STACK *stack);
static void	_clear(Q_STACK *stack);
static bool	_debug(Q_STACK *stack, FILE *out);
static void	_free(Q_STACK *stack);

#endif

/**
 * Create new Q_STACK stack container
 *
 * @return	a pointer of malloced Q_STACK container, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @code
 *   Q_STACK *stack = qStack();
 * @endcode
 */
Q_STACK *qStack(void) {
	Q_STACK *stack = (Q_STACK *)malloc(sizeof(Q_STACK));
	if(stack == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	memset((void *)stack, 0, sizeof(Q_STACK));
	stack->list = qList();
	if(stack->list == NULL) {
		free(stack);
		return NULL;
	}

	// methods
	stack->setSize		= _setSize;

	stack->push		= _push;
	stack->pushStr		= _pushStr;
	stack->pushInt		= _pushInt;

	stack->pop		= _pop;
	stack->popStr		= _popStr;
	stack->popInt		= _popInt;
	stack->popAt		= _popAt;

	stack->get		= _get;
	stack->getStr		= _getStr;
	stack->getInt		= _getInt;
	stack->getAt		= _getAt;

	stack->size		= _size;
	stack->clear		= _clear;
	stack->debug		= _debug;
	stack->free		= _free;

	return stack;
}

/**
 * Q_STACK->setSize(): Sets maximum number of elements allowed in this stack.
 *
 * @param stack	Q_STACK container pointer.
 * @param max	maximum number of elements. 0 means no limit.
 *
 * @return	previous maximum number.
 */
static size_t _setSize(Q_STACK *stack, size_t max) {
	return stack->list->setSize(stack->list, max);
}

/**
 * Q_STACK->push(): Pushes an element onto the top of this stack.
 *
 * @param stack	Q_STACK container pointer.
 * @param data	a pointer which points data memory.
 * @param size	size of the data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 * 	- ENOBUFS	: Stack full. Only happens when this stack has set to have limited number of elements)
 *	- ENOMEM	: Memory allocation failure.
 */
static bool _push(Q_STACK *stack, const void *data, size_t size) {
	return stack->list->addFirst(stack->list, data, size);
}

/**
 * Q_STACK->pushStr(): Pushes a string onto the top of this stack.
 *
 * @param stack	Q_STACK container pointer.
 * @param data	a pointer which points data memory.
 * @param size	size of the data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 * 	- ENOBUFS	: Stack full. Only happens when this stack has set to have limited number of elements.
 *	- ENOMEM	: Memory allocation failure.
 */
static bool _pushStr(Q_STACK *stack, const char *str) {
	if(str == NULL) {
		errno = EINVAL;
		return false;
	}
	return stack->list->addFirst(stack->list, str, strlen(str) + 1);
}

/**
 * Q_STACK->pushInt(): Pushes a integer onto the top of this stack.
 *
 * @param stack	Q_STACK container pointer.
 * @param num	integer data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 * 	- ENOBUFS	: Stack full. Only happens when this stack has set to have limited number of elements.
 *	- ENOMEM	: Memory allocation failure.
 */
static bool _pushInt(Q_STACK *stack, int num) {
	return stack->list->addFirst(stack->list, &num, sizeof(int));
}

/**
 * Q_STACK->pop(): Removes a element at the top of this stack and returns that element.
 *
 * @param stack	Q_STACK container pointer.
 * @param size	if size is not NULL, element size will be stored.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Stack is empty.
 *	- ENOMEM	: Memory allocation failure.
 */
static void *_pop(Q_STACK *stack, size_t *size) {
	return stack->list->popFirst(stack->list, size);
}

/**
 * Q_STACK->popStr(): Removes a element at the top of this stack and returns that element.
 *
 * @param stack	Q_STACK container pointer.
 *
 * @return	a pointer of malloced string element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Stack is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushStr().
 */
static char *_popStr(Q_STACK *stack) {
	size_t strsize;
	char *str = stack->list->popFirst(stack->list, &strsize);
	if(str != NULL) {
		str[strsize - 1] = '\0'; // just to make sure
	}

	return str;
}

/**
 * Q_STACK->popInt(): Removes a integer at the top of this stack and returns that element.
 *
 * @param stack	Q_STACK container pointer.
 *
 * @return	an integer value, otherwise returns 0.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Stack is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through pushInt().
 */
static int _popInt(Q_STACK *stack) {
	int num = 0;
	int *pnum = stack->list->popFirst(stack->list, NULL);
	if(pnum != NULL) {
		num = *pnum;
		free(pnum);
	}

	return num;
}

/**
 * Q_STACK->popAt(): Returns and remove the element at the specified position in this stack.
 *
 * @param stack	Q_STACK container pointer.
 * @param index	index at which the specified element is to be inserted
 * @param size	if size is not NULL, element size will be stored.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ERANGE	: Index out of range.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * Negative index can be used for addressing a element from the bottom in this stack.
 * For example, index -1 will always pop a element which is pushed at very first time.
 */
static void *_popAt(Q_STACK *stack, int index, size_t *size) {
	return stack->list->popAt(stack->list, index, size);
}

/**
 * Q_STACK->get(): Returns an element at the top of this stack without removing it.
 *
 * @param stack		Q_STACK container pointer.
 * @param size		if size is not NULL, element size will be stored.
 * @param newmem	whether or not to allocate memory for the element.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Stack is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 */
static void *_get(Q_STACK *stack, size_t *size, bool newmem) {
	return stack->list->getFirst(stack->list, size, newmem);
}

/**
 * Q_STACK->getStr(): Returns an string at the top of this stack without removing it.
 *
 * @param stack	Q_STACK container pointer.
 *
 * @return	a pointer of malloced string element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Stack is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushStr().
 */
static char *_getStr(Q_STACK *stack) {
	size_t strsize;
	char *str = stack->list->getFirst(stack->list, &strsize, true);
	if(str != NULL) {
		str[strsize - 1] = '\0'; // just to make sure
	}

	return str;
}

/**
 * Q_STACK->getInt(): Returns an integer at the top of this stack without removing it.
 *
 * @param stack	Q_STACK container pointer.
 *
 * @return	an integer value, otherwise returns 0.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Stack is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through pushInt().
 */
static int _getInt(Q_STACK *stack) {
	int num = 0;
	int *pnum = stack->list->getFirst(stack->list, NULL, true);
	if(pnum != NULL) {
		num = *pnum;
		free(pnum);
	}

	return num;
}

/**
 * Q_STACK->getAt(): Returns an element at the specified position in this stack without removing it.
 *
 * @param stack		Q_STACK container pointer.
 * @param index		index at which the specified element is to be inserted
 * @param size		if size is not NULL, element size will be stored.
 * @param newmem	whether or not to allocate memory for the element.
 *
 * @return	a pointer of element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ERANGE	: Index out of range.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * Negative index can be used for addressing a element from the bottom in this stack.
 * For example, index -1 will always get a element which is pushed at very first time.
 */
static void *_getAt(Q_STACK *stack, int index, size_t *size, bool newmem) {
	return stack->list->getAt(stack->list, index, size, newmem);
}

/**
 * Q_STACK->size(): Returns the number of elements in this stack.
 *
 * @param stack	Q_STACK container pointer.
 *
 * @return	the number of elements in this stack.
 */
static size_t _size(Q_STACK *stack) {
	return stack->list->size(stack->list);
}

/**
 * Q_STACK->clear(): Removes all of the elements from this stack.
 *
 * @param stack	Q_STACK container pointer.
 */
static void _clear(Q_STACK *stack) {
	stack->list->clear(stack->list);
}

/**
 * Q_STACK->debug(): Print out stored elements for debugging purpose.
 *
 * @param stack		Q_STACK container pointer.
 * @param out		output stream FILE descriptor such like stdout, stderr.
 *
 * @return		true if successful, otherwise returns false.
 */
static bool _debug(Q_STACK *stack, FILE *out) {
	return stack->list->debug(stack->list, out);
}

/**
 * Q_STACK->free(): Free Q_STACK
 *
 * @param stack	Q_STACK container pointer.
 *
 * @return	always returns true.
 */
static void _free(Q_STACK *stack) {
	stack->list->free(stack->list);
	free(stack);
}
