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
 * @file qQueue.c Queue implementation.
 *
 * Q_QUEUE container is a queue implementation. It represents a first-in-first-out(FIFO).
 * It extends container Q_LIST that allow a linked-list to be treated as a queue.
 *
 * @code
 *   [Conceptional Data Structure Diagram]
 *
 *                       top     bottom
 *   DATA POP    <==    [ A ][ B ][ C ]  <== DATA PUSH
 *   (positive index)     0    1    2
 *   (negative index)    -3   -2   -1
 * @endcode
 *
 * @code
 *   // create queue
 *   Q_QUEUE *queue = qQueue();
 *
 *   // example: integer queue
 *   queue->pushInt(queue, 1);
 *   queue->pushInt(queue, 2);
 *   queue->pushInt(queue, 3);
 *
 *   printf("popInt(): %d\n", queue->popInt(queue));
 *   printf("popInt(): %d\n", queue->popInt(queue));
 *   printf("popInt(): %d\n", queue->popInt(queue));
 *
 *   // example: string queue
 *   queue->pushStr(queue, "A string");
 *   queue->pushStr(queue, "B string");
 *   queue->pushStr(queue, "C string");
 *
 *   char *str = queue->popStr(queue);
 *   printf("popStr(): %s\n", str);
 *   free(str);
 *   str = queue->popStr(queue);
 *   printf("popStr(): %s\n", str);
 *   free(str);
 *   str = queue->popStr(queue);
 *   printf("popStr(): %s\n", str);
 *   free(str);
 *
 *   // example: object queue
 *   queue->push(queue, "A object", sizeof("A object"));
 *   queue->push(queue, "B object", sizeof("B object"));
 *   queue->push(queue, "C object", sizeof("C object"));
 *
 *   void *obj = queue->pop(queue, NULL);
 *   printf("pop(): %s\n", (char*)obj);
 *   free(obj);
 *   str = queue->pop(queue, NULL);
 *   printf("pop(): %s\n", (char*)obj);
 *   free(obj);
 *   str = queue->pop(queue, NULL);
 *   printf("pop(): %s\n", (char*)obj);
 *   free(obj);
 *
 *   // release
 *   queue->free(queue);
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

static size_t	_setSize(Q_QUEUE *queue, size_t max);

static bool	_push(Q_QUEUE *queue, const void *data, size_t size);
static bool	_pushStr(Q_QUEUE *queue, const char *str);
static bool	_pushInt(Q_QUEUE *queue, int num);

static void*	_pop(Q_QUEUE *queue, size_t *size);
static char*	_popStr(Q_QUEUE *queue);
static int	_popInt(Q_QUEUE *queue);
static void*	_popAt(Q_QUEUE *queue, int index, size_t *size);

static void*	_get(Q_QUEUE *queue, size_t *size, bool newmem);
static char*	_getStr(Q_QUEUE *queue);
static int	_getInt(Q_QUEUE *queue);
static void*	_getAt(Q_QUEUE *queue, int index, size_t *size, bool newmem);

static size_t	_size(Q_QUEUE *queue);
static void	_clear(Q_QUEUE *queue);
static bool	_debug(Q_QUEUE *queue, FILE *out);
static void	_free(Q_QUEUE *queue);

#endif

/**
 * Create new Q_QUEUE queue container
 *
 * @return	a pointer of malloced Q_QUEUE container, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @code
 *   Q_QUEUE *queue = qQueue();
 * @endcode
 */
Q_QUEUE *qQueue(void) {
	Q_QUEUE *queue = (Q_QUEUE *)malloc(sizeof(Q_QUEUE));
	if(queue == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	memset((void *)queue, 0, sizeof(Q_QUEUE));
	queue->list = qList();
	if(queue->list == NULL) {
		free(queue);
		return NULL;
	}

	// methods
	queue->setSize		= _setSize;

	queue->push		= _push;
	queue->pushStr		= _pushStr;
	queue->pushInt		= _pushInt;

	queue->pop		= _pop;
	queue->popStr		= _popStr;
	queue->popInt		= _popInt;
	queue->popAt		= _popAt;

	queue->get		= _get;
	queue->getStr		= _getStr;
	queue->getInt		= _getInt;
	queue->getAt		= _getAt;

	queue->size		= _size;
	queue->clear		= _clear;
	queue->debug		= _debug;
	queue->free		= _free;

	return queue;
}

/**
 * Q_QUEUE->setSize(): Sets maximum number of elements allowed in this queue.
 *
 * @param queue	Q_QUEUE container pointer.
 * @param max	maximum number of elements. 0 means no limit.
 *
 * @return	previous maximum number.
 */
static size_t _setSize(Q_QUEUE *queue, size_t max) {
	return queue->list->setSize(queue->list, max);
}

/**
 * Q_QUEUE->push(): Pushes an element onto the top of this queue.
 *
 * @param queue	Q_QUEUE container pointer.
 * @param data	a pointer which points data memory.
 * @param size	size of the data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 * 	- ENOBUFS	: Queue full. Only happens when this queue has set to have limited number of elements)
 *	- ENOMEM	: Memory allocation failure.
 */
static bool _push(Q_QUEUE *queue, const void *data, size_t size) {
	return queue->list->addLast(queue->list, data, size);
}

/**
 * Q_QUEUE->pushStr(): Pushes a string onto the top of this queue.
 *
 * @param queue	Q_QUEUE container pointer.
 * @param data	a pointer which points data memory.
 * @param size	size of the data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 * 	- ENOBUFS	: Queue full. Only happens when this queue has set to have limited number of elements.
 *	- ENOMEM	: Memory allocation failure.
 */
static bool _pushStr(Q_QUEUE *queue, const char *str) {
	if(str == NULL) {
		errno = EINVAL;
		return false;
	}
	return queue->list->addLast(queue->list, str, strlen(str) + 1);
}

/**
 * Q_QUEUE->pushInt(): Pushes a integer onto the top of this queue.
 *
 * @param queue	Q_QUEUE container pointer.
 * @param num	integer data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 * 	- ENOBUFS	: Queue full. Only happens when this queue has set to have limited number of elements.
 *	- ENOMEM	: Memory allocation failure.
 */
static bool _pushInt(Q_QUEUE *queue, int num) {
	return queue->list->addLast(queue->list, &num, sizeof(int));
}

/**
 * Q_QUEUE->pop(): Removes a element at the top of this queue and returns that element.
 *
 * @param queue	Q_QUEUE container pointer.
 * @param size	if size is not NULL, element size will be stored.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Queue is empty.
 *	- ENOMEM	: Memory allocation failure.
 */
static void *_pop(Q_QUEUE *queue, size_t *size) {
	return queue->list->popFirst(queue->list, size);
}

/**
 * Q_QUEUE->popStr(): Removes a element at the top of this queue and returns that element.
 *
 * @param queue	Q_QUEUE container pointer.
 *
 * @return	a pointer of malloced string element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Queue is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushStr().
 */
static char *_popStr(Q_QUEUE *queue) {
	size_t strsize;
	char *str = queue->list->popFirst(queue->list, &strsize);
	if(str != NULL) {
		str[strsize - 1] = '\0'; // just to make sure
	}

	return str;
}

/**
 * Q_QUEUE->popInt(): Removes a integer at the top of this queue and returns that element.
 *
 * @param queue	Q_QUEUE container pointer.
 *
 * @return	an integer value, otherwise returns 0.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Queue is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through pushInt().
 */
static int _popInt(Q_QUEUE *queue) {
	int num = 0;
	int *pnum = queue->list->popFirst(queue->list, NULL);
	if(pnum != NULL) {
		num = *pnum;
		free(pnum);
	}

	return num;
}

/**
 * Q_QUEUE->popAt(): Returns and remove the element at the specified position in this queue.
 *
 * @param queue	Q_QUEUE container pointer.
 * @param index	index at which the specified element is to be inserted
 * @param size	if size is not NULL, element size will be stored.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ERANGE	: Index out of range.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * Negative index can be used for addressing a element from the bottom in this queue.
 * For example, index -1 will always pop a element which is pushed at very last time.
 */
static void *_popAt(Q_QUEUE *queue, int index, size_t *size) {
	return queue->list->popAt(queue->list, index, size);
}

/**
 * Q_QUEUE->get(): Returns an element at the top of this queue without removing it.
 *
 * @param queue		Q_QUEUE container pointer.
 * @param size		if size is not NULL, element size will be stored.
 * @param newmem	whether or not to allocate memory for the element.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Queue is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 */
static void *_get(Q_QUEUE *queue, size_t *size, bool newmem) {
	return queue->list->getFirst(queue->list, size, newmem);
}

/**
 * Q_QUEUE->getStr(): Returns an string at the top of this queue without removing it.
 *
 * @param queue	Q_QUEUE container pointer.
 *
 * @return	a pointer of malloced string element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Queue is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushStr().
 */
static char *_getStr(Q_QUEUE *queue) {
	size_t strsize;
	char *str = queue->list->getFirst(queue->list, &strsize, true);
	if(str != NULL) {
		str[strsize - 1] = '\0'; // just to make sure
	}

	return str;
}

/**
 * Q_QUEUE->getInt(): Returns an integer at the top of this queue without removing it.
 *
 * @param queue	Q_QUEUE container pointer.
 *
 * @return	an integer value, otherwise returns 0.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: Queue is empty.
 *	- ENOMEM	: Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through pushInt().
 */
static int _getInt(Q_QUEUE *queue) {
	int num = 0;
	int *pnum = queue->list->getFirst(queue->list, NULL, true);
	if(pnum != NULL) {
		num = *pnum;
		free(pnum);
	}

	return num;
}

/**
 * Q_QUEUE->getAt(): Returns an element at the specified position in this queue without removing it.
 *
 * @param queue		Q_QUEUE container pointer.
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
 * Negative index can be used for addressing a element from the bottom in this queue.
 * For example, index -1 will always get a element which is pushed at very last time.
 */
static void *_getAt(Q_QUEUE *queue, int index, size_t *size, bool newmem) {
	return queue->list->getAt(queue->list, index, size, newmem);
}

/**
 * Q_QUEUE->size(): Returns the number of elements in this queue.
 *
 * @param queue	Q_QUEUE container pointer.
 *
 * @return	the number of elements in this queue.
 */
static size_t _size(Q_QUEUE *queue) {
	return queue->list->size(queue->list);
}

/**
 * Q_QUEUE->clear(): Removes all of the elements from this queue.
 *
 * @param queue	Q_QUEUE container pointer.
 */
static void _clear(Q_QUEUE *queue) {
	queue->list->clear(queue->list);
}

/**
 * Q_QUEUE->debug(): Print out stored elements for debugging purpose.
 *
 * @param queue		Q_QUEUE container pointer.
 * @param out		output stream FILE descriptor such like stdout, stderr.
 *
 * @return		true if successful, otherwise returns false.
 */
static bool _debug(Q_QUEUE *queue, FILE *out) {
	return queue->list->debug(queue->list, out);
}

/**
 * Q_QUEUE->free(): Free Q_QUEUE
 *
 * @param queue	Q_QUEUE container pointer.
 *
 * @return	always returns true.
 */
static void _free(Q_QUEUE *queue) {
	queue->list->free(queue->list);
	free(queue);
}
