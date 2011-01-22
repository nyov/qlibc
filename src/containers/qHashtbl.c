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
 * @file qHashtbl.c Hash-table container implementation.
 *
 * Q_HASHTBL implements a hashtable, which maps keys to values. Key is a unique string and value is any non-null object.
 * The creator qHashtbl() has one parameters that affect its performance: initial hash range.
 * The hash range is the number of slots(pointers) in the hash table. in the case of a hash collision,
 * a single slots stores multiple elements using linked-list structure, which must be searched sequentially.
 * So lower range than the number of elements decreases the space overhead but increases the number of hash collisions
 * and consequently it increases the time cost to look up an element.
 *
 * @code
 *   [Internal Structure Example for 10-slot hash table]
 *
 *   RANGE    NAMED-OBJECT-LIST
 *   =====    =================
 *   [ 0 ] -> [hash=320,key3=value] -> [hash=210,key5=value] -> [hash=110,key2=value]
 *   [ 1 ] -> [hash=1,key1=value]
 *   [ 2 ]
 *   [ 3 ] -> [hash=873,key4=value]
 *   [ 4 ] -> [hash=2674,key11=value] -> [hash=214,key5=value]
 *   [ 5 ] -> [hash=8545,key10=value]
 *   [ 6 ] -> [hash=9226,key9=value]
 *   [ 7 ]
 *   [ 8 ] -> [hash=8,key6=value] -> [hash=88,key8=value]
 *   [ 9 ] -> [hash=12439,key7=value]
 * @endcode
 *
 * @code
 *   // create a hash-table with 10 hash-index range.
 *   // Please beaware, the hash-index range 10 does not mean the number of objects which can be stored.
 *   // You can put as many as you want.
 *   Q_HASHTBL *tbl = qHashtbl(10);
 *
 *   // put objects into table.
 *   tbl->put(tbl, "sample1", "binary", 6);
 *   tbl->putStr(tbl, "sample2", "string");
 *   tbl->putInt(tbl, "sample3", 1);
 *
 *   // debug print out
 *   tbl->debug(tbl, stdout, true);
 *
 *   // get objects
 *   void *sample1 = tbl->get(tbl, "sample1", &size, true);
 *   char *sample2 = tbl->getStr(tbl, "sample2", false);
 *   int  sample3  = tbl->getInt(tbl, "sample3");
 *
 *   // sample1 is memalloced
 *   if(sample1 != NULL) free(sample1);
 *
 *   // release table
 *   tbl->free(tbl);
 * @endcode
 *
 * @note
 * Use "--enable-threadsafe" configure script option to use under multi-threaded environments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "qlibc.h"
#include "qInternal.h"

#ifndef _DOXYGEN_SKIP

// member methods
static bool	_put(Q_HASHTBL *tbl, const char *name, const void *data, size_t size);
static bool	_putStr(Q_HASHTBL *tbl, const char *name, const char *str);
static bool	_putStrf(Q_HASHTBL *tbl, const char *name, const char *format, ...);
static bool	_putInt(Q_HASHTBL *tbl, const char *name, int num);

static void*	_get(Q_HASHTBL *tbl, const char *name, size_t *size, bool newmem);
static char*	_getStr(Q_HASHTBL *tbl, const char *name, bool newmem);
static int	_getInt(Q_HASHTBL *tbl, const char *name);

static bool	_getNext(Q_HASHTBL *tbl, Q_NHLOBJ_T *obj, bool newmem);

static bool	_remove(Q_HASHTBL *tbl, const char *name);

static size_t	_size(Q_HASHTBL *tbl);
static void	_clear(Q_HASHTBL *tbl);
static bool	_debug(Q_HASHTBL *tbl, FILE *out);

static void	_lock(Q_HASHTBL *tbl);
static void	_unlock(Q_HASHTBL *tbl);

static void	_free(Q_HASHTBL *tbl);

#endif

/**
 * Initialize hash table.
 *
 * @param range		initial hash range.
 *
 * @return		a pointer of malloced Q_HASHTBL, otherwise returns false
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   // create a hash-table with hash-index range 1000 (does not mean maximum number of objects).
 *   Q_HASHTBL *tbl = qHashtbl(1000);
 * @endcode
 */
Q_HASHTBL *qHashtbl(size_t range) {
	if(range == 0) {
		errno = EINVAL;
		return NULL;
	}

	Q_HASHTBL *tbl = (Q_HASHTBL *)malloc(sizeof(Q_HASHTBL));
	if(tbl == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memset((void *)tbl, 0, sizeof(Q_HASHTBL));

	// allocate table space
	tbl->slots = (Q_NHLOBJ_T**)malloc(sizeof(Q_NHLOBJ_T*) * range);
	if(tbl->slots == NULL) {
		errno = ENOMEM;
		_free(tbl);
		return NULL;
	}
	memset((void *)tbl->slots, 0, sizeof(Q_NHLOBJ_T*) * range);

	// assign methods
	tbl->put	= _put;
	tbl->putStr	= _putStr;
	tbl->putStrf	= _putStrf;
	tbl->putInt	= _putInt;

	tbl->get	= _get;
	tbl->getStr	= _getStr;
	tbl->getInt	= _getInt;

	tbl->getNext	= _getNext;

	tbl->remove	= _remove;

	tbl->size	= _size;
	tbl->clear	= _clear;
	tbl->debug	= _debug;

	tbl->lock	= _lock;
	tbl->unlock	= _unlock;

	tbl->free	= _free;

	// initialize recrusive mutex
	Q_MUTEX_INIT(tbl->qmutex, true);

	// now table can be used
	tbl->range = range;

	return tbl;
}

/**
 * Q_HASHTBL->put(): Put a object into this table.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name
 * @param data		data object
 * @param size		size of data object
 *
 * @return		true if successful, otherwise returns false
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _put(Q_HASHTBL *tbl, const char *name, const void *data, size_t size) {
	if(name == NULL || data == NULL) {
		errno = EINVAL;
		return false;
	}

	// get hash integer
	uint32_t hash = qHashFnv32(name, strlen(name));
	int idx = hash % tbl->range;

	_lock(tbl);

	// find existence key
	Q_NHLOBJ_T *obj;
	for(obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
		if(obj->hash == hash && !strcmp(obj->name, name)) {
			break;
		}
	}

	// duplicate object
	char *dupname = strdup(name);
	void *dupdata = malloc(size);
	if(dupname == NULL || dupdata == NULL) {
		if(dupname != NULL) free(dupname);
		if(dupdata != NULL) free(dupdata);
		_unlock(tbl);
		errno = ENOMEM;
		return false;
	}
	memcpy(dupdata, data, size);

	// put into table
	if(obj == NULL) {
		// insert
		obj = (Q_NHLOBJ_T*)malloc(sizeof(Q_NHLOBJ_T));
		if(obj == NULL) {
			free(dupname);
			free(dupdata);
			_unlock(tbl);
			errno = ENOMEM;
			return false;
		}
		memset((void*)obj, 0, sizeof(Q_NHLOBJ_T));

		if(tbl->slots[idx] != NULL) {
			// insert at the beginning
			obj->next = tbl->slots[idx];
		}
		tbl->slots[idx] = obj;

		// increase counter
		tbl->num++;
	} else {
		// replace
		free(obj->name);
		free(obj->data);
	}

	// set data
	obj->hash = hash;
	obj->name = dupname;
	obj->data = dupdata;
	obj->size = size;

	_unlock(tbl);
	return true;
}

/**
 * Q_HASHTBL->putStr(): Put a string into this table.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name.
 * @param str		string data.
 *
 * @return		true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _putStr(Q_HASHTBL *tbl, const char *name, const char *str) {
	size_t size = (str != NULL) ? (strlen(str) + 1) : 0;
	return _put(tbl, name, str, size);
}

/**
 * Q_HASHTBL->putStrf(): Put a formatted string into this table.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name.
 * @param format	formatted string data.
 *
 * @return		true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _putStrf(Q_HASHTBL *tbl, const char *name, const char *format, ...) {
	char *str;
	DYNAMIC_VSPRINTF(str, format);
	if(str == NULL) {
		errno = ENOMEM;
		return false;
	}

	bool ret = _putStr(tbl, name, str);
	free(str);
	return ret;
}

/**
 * Q_HASHTBL->putInt(): Put a integer into this table as string type.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name.
 * @param num		integer data.
 *
 * @return		true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * The integer will be converted to a string object and stored as string object.
 */
static bool _putInt(Q_HASHTBL *tbl, const char *name, const int num) {
        char str[20+1];
        snprintf(str, sizeof(str), "%d", num);
	return _putStr(tbl, name, str);
}

/**
 * Q_HASHTBL->get(): Get a object from this table.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name.
 * @param size		if not NULL, oject size will be stored.
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return		a pointer of data if the key is found, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   Q_HASHTBL *tbl = qHashtbl(1000);
 *   (...codes...)
 *
 *   // with newmem flag unset
 *   size_t size;
 *   struct myobj *obj = (struct myobj*)tbl->get(tbl, "key_name", &size, false);
 *
 *   // with newmem flag set
 *   size_t size;
 *   struct myobj *obj = (struct myobj*)tbl->get(tbl, "key_name", &size, true);
 *   if(obj != NULL) free(obj);
 * @endcode
 *
 * @note
 * If newmem flag is set, returned data will be malloced and should be deallocated by user.
 * Otherwise returned pointer will point internal buffer directly and should not be de-allocated by user.
 * In thread-safe mode, newmem flag always should be true.
 */
static void *_get(Q_HASHTBL *tbl, const char *name, size_t *size, bool newmem) {
	if(name == NULL) {
		errno = EINVAL;
		return NULL;
	}

	uint32_t hash = qHashFnv32(name, strlen(name));
	int idx = hash % tbl->range;

	_lock(tbl);

	// find key
	Q_NHLOBJ_T *obj;
	for(obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
		if(obj->hash == hash && !strcmp(obj->name, name)) {
			break;
		}
	}

	void *data = NULL;
	if(obj != NULL) {
		if(newmem == false) {
			data = obj->data;
		} else {
			data = malloc(obj->size);
			if(data == NULL) {
				errno = ENOMEM;
				return NULL;
			}
			memcpy(data, obj->data, obj->size);
		}
		if(size != NULL && data != NULL) *size = obj->size;
	}

	_unlock(tbl);

	if(data == NULL) errno = ENOENT;
	return data;
}

/**
 * Q_HASHTBL->getStr(): Finds an object with given name and returns as string type.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return		a pointer of data if the key is found, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * If newmem flag is set, returned data will be malloced and should be deallocated by user.
 */
static char *_getStr(Q_HASHTBL *tbl, const char *name, bool newmem) {
	return _get(tbl, name, NULL, newmem);
}

/**
 * Q_HASHTBL->getInt(): Finds an object with given name and returns as integer type.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name
 *
 * @return		value integer if successful, otherwise(not found) returns 0
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static int _getInt(Q_HASHTBL *tbl, const char *name) {
	int num = 0;
	char *str = _getStr(tbl, name, true);
	if(str != NULL) {
		num = atoi(str);
		free(str);
	}

	return num;
}

/**
 * Q_HASHTBL->getNext(): Get next element.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param obj		found data will be stored in this object
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return		true if found otherwise returns false
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: No next element.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   Q_HASHTBL *tbl = qHashtbl(1000);
 *   (...add data into list...)
 *
 *   // non-thread usages
 *   Q_NHLOBJ_T obj;
 *   memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *   tbl->lock(tbl);
 *   while(tbl->getNext(tbl, &obj, false) == true) {
 *   	printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char*)obj.data, obj.size);
 *   }
 *   tbl->unlock(tbl);
 *
 *   // thread model with newmem flag
 *   Q_NHLOBJ_T obj;
 *   memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *   tbl->lock(tbl);
 *   while(tbl->getNext(tbl, &obj, true) == true) {
 *   	printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char*)obj.data, obj.size);
 *   	free(obj.name);
 *   	free(obj.data);
 *   }
 *   tbl->unlock(tbl);
 * @endcode
 *
 * @note
 * obj should be initialized with 0 by using memset() before first call.
 * If newmem flag is true, user should de-allocate obj.name and obj.data resources.
 */
static bool _getNext(Q_HASHTBL *tbl, Q_NHLOBJ_T *obj, bool newmem) {
	if(obj == NULL) {
		errno = EINVAL;
		return NULL;
	}

	_lock(tbl);

	bool found = false;

	Q_NHLOBJ_T *cursor = NULL;
	int idx = 0;
	if(obj->name != NULL) {
		idx = (obj->hash % tbl->range) + 1;
		cursor = obj->next;
	}

	if(cursor != NULL) {
		// has link
		found = true;
	} else {
		// search from next index
		for(; idx < tbl->range; idx++) {
			if(tbl->slots[idx] != NULL) {
				cursor = tbl->slots[idx];
				found = true;
				break;
			}
		}
	}

	if(cursor != NULL) {
		if(newmem == true) {
			obj->name = strdup(cursor->name);
			obj->data = malloc(cursor->size);
			if(obj->name == NULL || obj->data == NULL) {
				DEBUG("Q_HASHTBL->getNext(): Unable to allocate memory.");
				if(obj->name != NULL) free(obj->name);
				if(obj->data != NULL) free(obj->data);
				_unlock(tbl);
				errno = ENOMEM;
				return false;
			}
			memcpy(obj->data, cursor->data, cursor->size);
			obj->size = cursor->size;
		} else {
			obj->name = cursor->name;
			obj->data = cursor->data;
		}
		obj->hash = cursor->hash;
		obj->size = cursor->size;
		obj->next = cursor->next;

	}

	_unlock(tbl);

	if(found == false) errno = ENOENT;

	return found;
}

/**
 * Q_HASHTBL->remove(): Remove an object from this table.
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param name		key name
 *
 * @return		true if successful, otherwise(not found) returns false
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: No next element.
 *	- EINVAL	: Invalid argument.
 */
static bool _remove(Q_HASHTBL *tbl, const char *name) {
	if(name == NULL) {
		errno = EINVAL;
		return false;
	}

	_lock(tbl);

	uint32_t hash = qHashFnv32(name, strlen(name));
	int idx = hash % tbl->range;

	// find key
	bool found = false;
	Q_NHLOBJ_T *prev = NULL;
	Q_NHLOBJ_T *obj;
	for(obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
		if(obj->hash == hash && !strcmp(obj->name, name)) {
			// adjust link
			if(prev == NULL) tbl->slots[idx] = obj->next;
			else prev->next = obj->next;

			// remove
			free(obj->name);
			free(obj->data);
			free(obj);

			found = true;
			tbl->num--;
			break;
		}

		prev = obj;
	}

	_unlock(tbl);

	if(found == false) errno = ENOENT;

	return found;
}

/**
 * Q_HASHTBL->size(): Returns the number of keys in this hashtable.
 *
 * @param tbl		Q_HASHTBL container pointer.
 *
 * @return		number of elements stored
 */
static size_t _size(Q_HASHTBL *tbl) {
	return tbl->num;
}

/**
 * Q_HASHTBL->clear(): Clears this hashtable so that it contains no keys.
 *
 * @param tbl		Q_HASHTBL container pointer.
 */
void _clear(Q_HASHTBL *tbl) {
	_lock(tbl);
	int idx;
	for (idx = 0; idx < tbl->range && tbl->num > 0; idx++) {
		if(tbl->slots[idx] == NULL) continue;
		Q_NHLOBJ_T *obj = tbl->slots[idx];
		tbl->slots[idx] = NULL;
		while(obj != NULL) {
			Q_NHLOBJ_T *next = obj->next;
			free(obj->name);
			free(obj->data);
			free(obj);
			obj = next;

			tbl->num--;
		}
	}

	_unlock(tbl);
}

/**
 * Q_HASHTBL->debug(): Print hash table for debugging purpose
 *
 * @param tbl		Q_HASHTBL container pointer.
 * @param out		output stream
 *
 * @return		true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EIO		: Invalid output stream.
 */
bool _debug(Q_HASHTBL *tbl, FILE *out) {
	if(out == NULL) {
		errno = EIO;
		return false;
	}

 	Q_NHLOBJ_T obj;
 	memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 	_lock(tbl);
	while(tbl->getNext(tbl, &obj, false) == true) {
		fprintf(out, "%s=" , obj.name);
		_q_humanOut(out, obj.data, obj.size, MAX_HUMANOUT);
		fprintf(out, " (%zu, hash=%u)\n" , obj.size, obj.hash);
 	}
 	_unlock(tbl);

	return true;
}

/**
 * Q_HASHTBL->lock(): Enter critical section.
 *
 * @param tbl		Q_HASHTBL container pointer.
 *
 * @note
 * From user side, normally locking operation is only needed when traverse all elements using Q_HASHTBL->getNext().
 * Most of other operations do necessary locking internally when it's compiled with --enable-threadsafe option.
 *
 * @note
 * This operation will be activated only when --enable-threadsafe option is given at compile time.
 * To find out whether it's compiled with threadsafe option, call qLibcThreadsafe().
 */
static void _lock(Q_HASHTBL *tbl) {
	Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * Q_HASHTBL->unlock(): Leave critical section.
 *
 * @param tbl		Q_HASHTBL container pointer.
 *
 * @note
 * This operation will be activated only when --enable-threadsafe option is given at compile time.
 * To find out whether it's compiled with threadsafe option, call qLibcThreadsafe().
 */
static void _unlock(Q_HASHTBL *tbl) {
	Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * Q_HASHTBL->free(): De-allocate hash table
 *
 * @param tbl		Q_HASHTBL container pointer.
 */
void _free(Q_HASHTBL *tbl) {
	_lock(tbl);
	_clear(tbl);
	if(tbl->slots != NULL) free(tbl->slots);
	_unlock(tbl);
	Q_MUTEX_DESTROY(tbl->qmutex);

	free(tbl);
}
