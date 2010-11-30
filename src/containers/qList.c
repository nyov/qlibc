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
 * @file qList.c Linked-list implementation.
 *
 * Q_LIST container is a doubly Linked-List implementation.
 * Q_LIST provides uniformly named methods to add, get, pop and remove an element at the beginning and end of the list.
 * These operations allow Q_LIST to be used as a stack, queue, or double-ended queue.
 *
 * @code
 *   [Conceptional Data Structure Diagram]
 *
 *   last~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
 *                                                           |
 *           +-----------+  doubly  +-----------+  doubly  +-|---------+
 *   first~~~|~>   0   <~|~~~~~~~~~~|~>   1   <~|~~~~~~~~~~|~>   N     |
 *           +-----|-----+  linked  +-----|-----+  linked  +-----|-----+
 *                 |                      |                      |
 *           +-----v---------------+      |                +-----v-----+
 *           | DATA A              |      |                | DATA N    |
 *           +---------------------+      |                +-----------+
 *                  +---------------------v------------------+
 *                  | DATA B                                 |
 *                  +----------------------------------------+
 * @endcode
 *
 * @code
 *   // create a list.
 *   Q_LIST *list = qList();
 *
 *   // insert elements
 *   list->addLast(list, "e1", sizeof("e1"));
 *   list->addLast(list, "e2", sizeof("e2"));
 *   list->addLast(list, "e3", sizeof("e3"));
 *
 *   // get
 *   char *e1 = (char*)list->getFirst(list, NULL, true));   // get malloced copy
 *   char *e3  = (char*)list->getAt(list, -1, NULL, false)); // get a direct pointer
 *   (...omit...)
 *   free(e1);
 *
 *   // pop (get and remove)
 *   char *e2 = (char*)list->popAt(list, 1, NULL)); // get malloced copy
 *   (...omit...)
 *   free(e2);
 *
 *   // debug output
 *   list->debug(list, stdout, true);
 *
 *   // travesal
 *   Q_DLOBJ_T obj;
 *   memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *   list->lock(list);
 *   while(list->getNext(list, &obj, false) == true) {
 *     printf("DATA=%s, SIZE=%zu\n", (char*)obj.data, obj.size);
 *   }
 *   list->unlock(list);
 *
 *   // free object
 *   list->free(list);
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
static size_t	_setSize(Q_LIST *list, size_t max);

static bool	_addFirst(Q_LIST *list, const void *data, size_t size);
static bool	_addLast(Q_LIST *list, const void *data, size_t size);
static bool	_addAt(Q_LIST *list, int index, const void *data, size_t size);

static void*	_getFirst(Q_LIST *list, size_t *size, bool newmem);
static void*	_getLast(Q_LIST *list, size_t *size, bool newmem);
static void*	_getAt(Q_LIST *list, int index, size_t *size, bool newmem);
static bool	_getNext(Q_LIST *list, Q_DLOBJ_T *obj, bool newmem);

static void*	_popFirst(Q_LIST *list, size_t *size);
static void*	_popLast(Q_LIST *list, size_t *size);
static void*	_popAt(Q_LIST *list, int index, size_t *size);

static bool	_removeFirst(Q_LIST *list);
static bool	_removeLast(Q_LIST *list);
static bool	_removeAt(Q_LIST *list, int index);

static size_t	_size(Q_LIST *list);
static size_t	_datasize(Q_LIST *list);
static void	_reverse(Q_LIST *list);
static void	_clear(Q_LIST *list);

static void*	_toArray(Q_LIST *list, size_t *size);
static char*	_toString(Q_LIST *list);
static bool	_debug(Q_LIST *list, FILE *out);

static void	_lock(Q_LIST *list);
static void	_unlock(Q_LIST *list);

static void	_free(Q_LIST *list);

/* internal functions */
static void*	_getAt2(Q_LIST *list, int index, size_t *size, bool newmem, bool remove);
static Q_DLOBJ_T* _getObjPt(Q_LIST *list, int index);
static bool	_removeObj(Q_LIST *list, Q_DLOBJ_T *obj);

#endif

/**
 * Create new Q_LIST linked-list container
 *
 * @return	a pointer of malloced Q_LIST container, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   Q_LIST *list = qList();
 * @endcode
 */
Q_LIST *qList(void) {
	Q_LIST *list = (Q_LIST *)malloc(sizeof(Q_LIST));
	if(list == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	/* initialize container */
	memset((void *)list, 0, sizeof(Q_LIST));

	/* member methods */
	list->setSize		= _setSize;

	list->addFirst		= _addFirst;
	list->addLast		= _addLast;
	list->addAt		= _addAt;

	list->getFirst		= _getFirst;
	list->getLast		= _getLast;
	list->getAt		= _getAt;
	list->getNext		= _getNext;

	list->popFirst		= _popFirst;
	list->popLast		= _popLast;
	list->popAt		= _popAt;

	list->removeFirst	= _removeFirst;
	list->removeLast	= _removeLast;
	list->removeAt		= _removeAt;

	list->size		= _size;
	list->datasize		= _datasize;
	list->reverse		= _reverse;
	list->clear		= _clear;

	list->toArray		= _toArray;
	list->toString		= _toString;
	list->debug		= _debug;

	list->lock		= _lock;
	list->unlock		= _unlock;

	list->free		= _free;

	// initialize recrusive mutex
	Q_MUTEX_INIT(list->qmutex, true);

	return list;
}

/**
 * Q_LIST->setSize(): Sets maximum number of elements allowed in this list.
 *
 * @param list	Q_LIST container pointer.
 * @param max	maximum number of elements. 0 means no limit.
 *
 * @return	previous maximum number.
 */
static size_t _setSize(Q_LIST *list, size_t max) {
	_lock(list);
	size_t old = list->max;
	list->max = max;
	_unlock(list);
	return old;
}

/**
 * Q_LIST->addFirst(): Inserts a element at the beginning of this list.
 *
 * @param list	Q_LIST container pointer.
 * @param data	a pointer which points data memory.
 * @param size	size of the data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 * 	- ENOBUFS	: List full. Only happens when this list has set to have limited number of elements.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   // create a sample object.
 *   struct my_obj obj;
 *
 *   // create a list and add the sample object.
 *   Q_LIST *list = qList();
 *   list->addFirst(list, &obj, sizeof(struct my_obj));
 * @endcode
 */
static bool _addFirst(Q_LIST *list, const void *data, size_t size) {
	return _addAt(list, 0, data, size);

}

/**
 * Q_LIST->addLast(): Appends a element to the end of this list.
 *
 * @param list	Q_LIST container pointer.
 * @param data	a pointer which points data memory.
 * @param size	size of the data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 * 	- ENOBUFS	: List full. Only happens when this list has set to have limited number of elements.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _addLast(Q_LIST *list, const void *data, size_t size) {
	return _addAt(list, -1, data, size);
}

/**
 * Q_LIST->addAt(): Inserts a element at the specified position in this list.
 *
 * @param list	Q_LIST container pointer.
 * @param index	index at which the specified element is to be inserted.
 * @param data	a pointer which points data memory.
 * @param size	size of the data.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 * 	- ENOBUFS	: List full. Only happens when this list has set to have limited number of elements.
 *	- ERANGE	: Index out of range.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *                      first           last      new
 *   Linked-list        [ A ]<=>[ B ]<=>[ C ]?==?[   ]
 *   (positive index)     0       1       2        3
 *   (negative index)    -3      -2      -1
 * @endcode
 *
 * @code
 *   list->addAt(list, 0, &obj, sizeof(obj));  // same as addFirst().
 *   list->addAt(list, -1, &obj, sizeof(obj)); // same as addLast().
 * @endcode
 *
 * @note
 * Index starts from 0.
 */
static bool _addAt(Q_LIST *list, int index, const void *data, size_t size) {
	// check arguments
	if(data == NULL || size <= 0) {
		errno = EINVAL;
		return false;
	}

	_lock(list);

	// check maximum number of allowed elements if set
	if(list->max > 0 && list->num >= list->max) {
		errno = ENOBUFS;
		_unlock(list);
		return false;
	}

	// adjust index
	if(index < 0) index = (list->num + index) + 1; // -1 is same as addLast()
	if(index < 0 || index > list->num) {
		// out of bound
		_unlock(list);
		errno = ERANGE;
		return false;
	}

	// duplicate object
	void *dup_data = malloc(size);
	if(dup_data == NULL) {
		_unlock(list);
		errno = ENOMEM;
		return false;
	}
	memcpy(dup_data, data, size);

	// make new object list
	Q_DLOBJ_T *obj = (Q_DLOBJ_T*)malloc(sizeof(Q_DLOBJ_T));
	if(obj == NULL) {
		free(dup_data);
		_unlock(list);
		errno = ENOMEM;
		return false;
	}
	obj->data = dup_data;
	obj->size = size;
	obj->prev = NULL;
	obj->next = NULL;

	// make link
	if(index == 0) {
		// add at first
		obj->next = list->first;
		if(obj->next != NULL) obj->next->prev = obj;
		list->first = obj;
		if(list->last == NULL) list->last = obj;
	} else if(index == list->num) {
		// add after last
		obj->prev = list->last;
		if(obj->prev != NULL) obj->prev->next = obj;
		list->last = obj;
		if(list->first == NULL) list->first = obj;
	} else {
		// add at the middle of list
		Q_DLOBJ_T *tgt = _getObjPt(list, index);
		if(tgt == NULL) {
			// should not be happened.
			free(dup_data);
			free(obj);
			_unlock(list);
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

	_unlock(list);

	return true;
}

/**
 * Q_LIST->getFirst(): Returns the first element in this list.
 *
 * @param list		Q_LIST container pointer.
 * @param size		if size is not NULL, element size will be stored.
 * @param newmem	whether or not to allocate memory for the element.
 *
 * @return	a pointer of element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   size_t size;
 *   void *data = list->getFirst(list, &size, true);
 *   if(data != NULL) {
 *     (...omit...)
 *     free(data);
 *   }
 * @endcode
 */
static void *_getFirst(Q_LIST *list, size_t *size, bool newmem) {
	return _getAt(list, 0, size, newmem);
}

/**
 * Q_LIST->getLast(): Returns the last element in this list.
 *
 * @param list		Q_LIST container pointer.
 * @param size		if size is not NULL, element size will be stored.
 * @param newmem	whether or not to allocate memory for the element.
 *
 * @return	a pointer of element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 */
static void *_getLast(Q_LIST *list, size_t *size, bool newmem) {
	return _getAt(list, -1, size, newmem);
}

/**
 * Q_LIST->getAt(): Returns the element at the specified position in this list.
 *
 * @param list		Q_LIST container pointer.
 * @param index		index at which the specified element is to be inserted
 * @param size		if size is not NULL, element size will be stored.
 * @param newmem	whether or not to allocate memory for the element.
 *
 * @return	a pointer of element, otherwise returns NULL.
 * @retval	errno
 * @retval	errno	will be set in error condition.
 *	- ERANGE	: Index out of range.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *                      first           last
 *   Linked-list        [ A ]<=>[ B ]<=>[ C ]
 *   (positive index)     0       1       2
 *   (negative index)    -3      -2      -1
 * @endcode
 *
 * @note
 * Negative index can be used for addressing a element from the end in this stack.
 * For example, index -1 is same as getLast() and index 0 is same as getFirst();
 */
static void *_getAt(Q_LIST *list, int index, size_t *size, bool newmem) {
	return _getAt2(list, index, size, newmem, false);
}

/**
 * Q_LIST->getNext(): Get next element in this list.
 *
 * @param list		Q_LIST container pointer.
 * @param obj		found data will be stored in this structure
 * @param newmem	whether or not to allocate memory for the element.
 *
 * @return	true if found otherwise returns false
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: No next element.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * obj should be initialized with 0 by using memset() before first call.
 * If newmem flag is true, user should de-allocate obj.name and obj.data resources.
 *
 * @code
 *   Q_LIST *list = qList();
 *   (...add data into list...)
 *
 *   Q_DLOBJ_T obj;
 *   memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *   list->lock(list);   // can be omitted in single thread model.
 *   while(list->getNext(list, &obj, false) == true) {
 *   	printf("DATA=%s, SIZE=%zu\n", (char*)obj.data, obj.size);
 *   }
 *   list->unlock(list); // release lock.
 * @endcode
 */
static bool _getNext(Q_LIST *list, Q_DLOBJ_T *obj, bool newmem) {
	if(obj == NULL) return NULL;

	_lock(list);

	Q_DLOBJ_T *cont = NULL;

	if(obj->prev == NULL && obj->next == NULL) cont = list->first;
	else cont = obj->next;

	if(cont == NULL) {
		errno = ENOENT;
		_unlock(list);
		return false;
	}

	bool ret = false;
	while(cont != NULL) {
		if(newmem == true) {
			obj->data = malloc(cont->size);
			if(obj->data == NULL) break;

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

	_unlock(list);
	return ret;
}

/**
 * Q_LIST->popFirst(): Returns and remove the first element in this list.
 *
 * @param list	Q_LIST container pointer.
 * @param size	if size is not NULL, element size will be stored.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 */
static void *_popFirst(Q_LIST *list, size_t *size) {
	return _popAt(list, 0, size);
}


/**
 * Q_LIST->getLast(): Returns and remove the last element in this list.
 *
 * @param list	Q_LIST container pointer.
 * @param size	if size is not NULL, element size will be stored.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 */
static void *_popLast(Q_LIST *list, size_t *size) {
	return _popAt(list, -1, size);
}

/**
 * Q_LIST->popAt(): Returns and remove the element at the specified position in this list.
 *
 * @param list	Q_LIST container pointer.
 * @param index	index at which the specified element is to be inserted
 * @param size	if size is not NULL, element size will be stored.
 *
 * @return	a pointer of malloced element, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ERANGE	: Index out of range.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *                      first           last
 *   Linked-list        [ A ]<=>[ B ]<=>[ C ]
 *   (positive index)     0       1       2
 *   (negative index)    -3      -2      -1
 * @endcode
 *
 * @note
 * Negative index can be used for addressing a element from the end in this stack.
 * For example, index -1 is same as popLast() and index 0 is same as popFirst();
 */
static void *_popAt(Q_LIST *list, int index, size_t *size) {
	return _getAt2(list, index, size, true, true);
}

/**
 * Q_LIST->removeFirst(): Removes the first element in this list.
 *
 * @param list	Q_LIST container pointer.
 *
 * @return	a number of removed objects.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 */
static bool _removeFirst(Q_LIST *list) {
	return _removeAt(list, 0);
}

/**
 * Q_LIST->removeLast(): Removes the last element in this list.
 *
 * @param list	Q_LIST container pointer.
 *
 * @return	a number of removed objects.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 */
static bool _removeLast(Q_LIST *list) {
	return _removeAt(list, -1);
}

/**
 * Q_LIST->removeAt(): Removes the element at the specified position in this list.
 *
 * @param list	Q_LIST container pointer.
 * @param index	index at which the specified element is to be removed.
 *
 * @return	a number of removed objects.
 * @retval	errno	will be set in error condition.
 *	- ERANGE	: Index out of range.
 */
static bool _removeAt(Q_LIST *list, int index) {
	_lock(list);

	// get object pointer
	Q_DLOBJ_T *obj = _getObjPt(list, index);
	if(obj == NULL) {
		_unlock(list);
		return false;
	}

	bool ret = _removeObj(list, obj);

	_unlock(list);

	return ret;
}

/**
 * Q_LIST->size(): Returns the number of elements in this list.
 *
 * @param list	Q_LIST container pointer.
 *
 * @return	the number of elements in this list.
 */
static size_t _size(Q_LIST *list) {
	return list->num;
}

/**
 * Q_LIST->size(): Returns the sum of total element size.
 *
 * @param list	Q_LIST container pointer.
 *
 * @return	the sum of total element size.
 */
static size_t _datasize(Q_LIST *list) {
	return list->datasum;
}

/**
 * Q_LIST->reverse(): Reverse the order of elements.
 *
 * @param list	Q_LIST container pointer.
 */
static void _reverse(Q_LIST *list) {
	_lock(list);
	Q_DLOBJ_T *obj;
	for (obj = list->first; obj;) {
		Q_DLOBJ_T *next = obj->next;
		obj->next = obj->prev;
		obj->prev = next;
		obj = next;
	}

	obj = list->first;
	list->first = list->last;
	list->last = obj;

	_unlock(list);
}

/**
 * Q_LIST->clear(): Removes all of the elements from this list.
 *
 * @param list	Q_LIST container pointer.
 */
static void _clear(Q_LIST *list) {
	_lock(list);
	Q_DLOBJ_T *obj;
	for(obj = list->first; obj;) {
		Q_DLOBJ_T *next = obj->next;
		free(obj->data);
		free(obj);
		obj = next;
	}

	list->num = 0;
	list->datasum = 0;
	list->first = NULL;
	list->last = NULL;
	_unlock(list);
}

/**
 * Q_LIST->toArray(): Returns the serialized chunk containing all the elements in this list.
 *
 * @param list	Q_LIST container pointer.
 * @param size	if size is not NULL, chunk size will be stored.
 *
 * @return	a malloced pointer, otherwise(if there is no data to merge) returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 */
static void *_toArray(Q_LIST *list, size_t *size) {
	if(list->num <= 0) {
		if(size != NULL) *size = 0;
		errno = ENOENT;
		return NULL;
	}

	_lock(list);

	void *chunk = malloc(list->datasum);
	if(chunk == NULL) {
		_unlock(list);
		errno = ENOMEM;
		return NULL;
	}
	void *dp = chunk;

	Q_DLOBJ_T *obj;
	for(obj = list->first; obj; obj = obj->next) {
		memcpy(dp, obj->data, obj->size);
		dp += obj->size;
	}
	_unlock(list);

	if(size != NULL) *size = list->datasum;
	return chunk;
}

/**
 * Q_LIST->toString(): Returns a string representation of this list, containing string representation of each element.
 *
 * @param list	Q_LIST container pointer.
 *
 * @return	a malloced pointer, otherwise(if there is no data to merge) returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * Return string is always terminated by '\0'.
 */
static char *_toString(Q_LIST *list) {
	if(list->num <= 0) {
		errno = ENOENT;
		return NULL;
	}

	_lock(list);

	void *chunk = malloc(list->datasum + 1);
	if(chunk == NULL) {
		_unlock(list);
		errno = ENOMEM;
		return NULL;
	}
	void *dp = chunk;

	Q_DLOBJ_T *obj;
	for(obj = list->first; obj; obj = obj->next) {
		size_t size = obj->size;
		if(*(char*)(obj->data + (size - 1)) == '\0') size -= 1; // do not copy tailing '\0'
		memcpy(dp, obj->data, size);
		dp += size;
	}
	*((char *)dp) = '\0';
	_unlock(list);

	return (char*)chunk;
}

/**
 * Q_LIST->debug(): Prints out stored elements for debugging purpose.
 *
 * @param list		Q_LIST container pointer.
 * @param out		output stream FILE descriptor such like stdout, stderr.
 *
 * @return		true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EIO		: Invalid output stream.
 */
static bool _debug(Q_LIST *list, FILE *out) {
	if(out == NULL) {
		errno = EIO;
		return false;
	}

	_lock(list);
	Q_DLOBJ_T *obj;
	int i;
	for(i = 0, obj = list->first; obj; obj = obj->next, i++) {
		fprintf(out, "%d=" , i);
		_q_humanOut(out, obj->data, obj->size, MAX_HUMANOUT);
		fprintf(out, " (%zu)\n" , obj->size);
	}
	_unlock(list);

	return true;
}

/**
 * Q_LIST->lock(): Enters critical section.
 *
 * @param list	Q_LIST container pointer.
 *
 * @note
 * From user side, normally locking operation is only needed when traverse all elements using Q_LIST->getNext().
 * Most of other operations do necessary locking internally when it's compiled with --enable-threadsafe option.
 *
 * @note
 * This operation will be activated only when --enable-threadsafe option is given at compile time.
 * To find out whether it's compiled with threadsafe option, call qLibcThreadsafe().
 */
static void _lock(Q_LIST *list) {
	Q_MUTEX_ENTER(list->qmutex);
}

/**
 * Q_LIST->unlock(): Leaves critical section.
 *
 * @param list	Q_LIST container pointer.
 *
 * @note
 * This operation will be activated only when --enable-threadsafe option is given at compile time.
 * To find out whether it's compiled with threadsafe option, call qLibcThreadsafe().
 */
static void _unlock(Q_LIST *list) {
	Q_MUTEX_LEAVE(list->qmutex);
}

/**
 * Q_LIST->free(): Free Q_LIST.
 *
 * @param list	Q_LIST container pointer.
 */
static void _free(Q_LIST *list) {
	_clear(list);
	Q_MUTEX_DESTROY(list->qmutex);

	free(list);
}

#ifndef _DOXYGEN_SKIP

static void *_getAt2(Q_LIST *list, int index, size_t *size, bool newmem, bool remove) {
	_lock(list);

	// get object pointer
	Q_DLOBJ_T *obj = _getObjPt(list, index);
	if(obj == NULL) {
		_unlock(list);
		return false;
	}

	// copy data
	void *data;
	if(newmem == true) {
		data = malloc(obj->size);
		if(data == NULL) {
			_unlock(list);
			errno = ENOMEM;
			return false;
		}
		memcpy(data, obj->data, obj->size);
	} else {
		data = obj->data;
	}
	if(size != NULL) *size = obj->size;

	// remove if necessary
	if(remove == true) {
		if(_removeObj(list, obj) == false) {
			if(newmem == true) free(data);
			data = NULL;
		}
	}

	_unlock(list);

	return data;
}

static Q_DLOBJ_T *_getObjPt(Q_LIST *list, int index) {
	// index adjustment
	if(index < 0) index = list->num + index;
	if(index >= list->num) {
		errno = ERANGE;
		return NULL;
	}

	// detect faster scan direction
	bool backward;
	Q_DLOBJ_T *obj;
	int listidx;
	if(index < list->num / 2) {
		backward = false;
		obj = list->first;
		listidx = 0;
	} else {
		backward = true;
		obj = list->last;
		listidx = list->num - 1;
	}

	// find object
	while(obj != NULL) {
		if(listidx == index) return obj;

		if(backward == false) {
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

static bool _removeObj(Q_LIST *list, Q_DLOBJ_T *obj) {
	if(obj == NULL) return false;

	// chain prev and next elements
	if(obj->prev == NULL) list->first = obj->next;
	else obj->prev->next = obj->next;
	if(obj->next == NULL) list->last = obj->prev;
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
