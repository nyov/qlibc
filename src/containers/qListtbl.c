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
 * @file qListtbl.c Linked-list-table implementation.
 *
 * Q_LISTTBL container is a Linked-List-Table implementation.
 * Which maps keys to values. Key is a string and value is any non-null object.
 * These elements are stored sequentially in Doubly-Linked-List data structure.
 * In addition, Q_LISTTBL allows to add multiple records with same keys.
 *
 * @code
 *   [Conceptional Data Structure Diagram]
 *
 *   last~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
 *                                                           |
 *           +-----------+  doubly  +-----------+  doubly  +-|---------+
 *   first~~~|~>   0   <~|~~~~~~~~~~|~>   1   <~|~~~~~~~~~~|~>   N     |
 *           +--|-----|--+  linked  +--|-----|--+  linked  +--|-----|--+
 *              |     |                |     |                |     |
 *        +-----v-+ +-v------------+   |     |          +-----v-+ +-v-------+
 *        | KEY A | | VALUE A      |   |     |          | KEY N | | VALUE N |
 *        +-------+ +--------------+   |     |          +-------+ +---------+
 *                  +------------------v-+ +-v---------------------+
 *                  | KEY B              | | VALUE B               |
 *                  +--------------------+ +-----------------------+
 * @endcode
 *
 * @code
 *   // create a list.
 *   Q_LISTTBL *tbl = qListtbl();
 *
 *   // insert elements (key duplication allowed)
 *   tbl->put(tbl, "e1", "a", strlen("e1")+1, false); // equal to addStr();
 *   tbl->putStr(tbl, "e2", "b", false);
 *   tbl->putStr(tbl, "e2", "c", false);
 *   tbl->putStr(tbl, "e3", "d", false);
 *
 *   // debug output
 *   tbl->debug(tbl, stdout, true);
 *
 *   // get element
 *   printf("get('e2') : %s\n", (char*)tbl->get(tbl, "e2", NULL, false));
 *   printf("getLast('e2') : %s\n", (char*)tbl->getLast(tbl, "e2", NULL, false));
 *   printf("getStr('e2') : %s\n", tbl->getStr(tbl, "e2", false));
 *
 *   // find every 'e2' elements
 *   Q_NDLOBJ_T obj;
 *   memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *   while(tbl->getNext(tbl, &obj, "e2", false) == true) {
 *     printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char*)obj.data, obj.size);
 *   }
 *
 *   // free object
 *   tbl->free(tbl);
 * @endcode
 *
 * @note
 * Use "--enable-threadsafe" configure script option to use under multi-threaded environments.
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

#define _VAR		'$'
#define _VAR_OPEN	'{'
#define _VAR_CLOSE	'}'
#define _VAR_CMD	'!'
#define _VAR_ENV	'%'

static bool		_setPutDirection(Q_LISTTBL *tbl, bool first);

static bool		_put(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique);
static bool		_putFirst(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique);
static bool		_putLast(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique);

static bool		_putStr(Q_LISTTBL *tbl, const char *name, const char *str, bool unique);
static bool		_putStrf(Q_LISTTBL *tbl, bool unique, const char *name, const char *format, ...);
static bool		_putInt(Q_LISTTBL *tbl, const char *name, int num, bool unique);

static void*		_get(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem);
static void*		_getLast(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem);

static char*		_getStr(Q_LISTTBL *tbl, const char *name, bool newmem);
static int		_getInt(Q_LISTTBL *tbl, const char *name);

static void*		_getCase(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem);
static char*		_getCaseStr(Q_LISTTBL *tbl, const char *name, bool newmem);
static int		_getCaseInt(Q_LISTTBL *tbl, const char *name);

static bool		_getNext(Q_LISTTBL *tbl, Q_NDLOBJ_T *obj, const char *name, bool newmem);

static bool		_remove(Q_LISTTBL *tbl, const char *name);

static size_t 		_size(Q_LISTTBL *tbl);
static void		_reverse(Q_LISTTBL *tbl);
static void		_clear(Q_LISTTBL *tbl);

static char*		_parseStr(Q_LISTTBL *tbl, const char *str);
static bool		_save(Q_LISTTBL *tbl, const char *filepath, char sepchar, bool encode);
static size_t		_load(Q_LISTTBL *tbl, const char *filepath, char sepchar, bool decode);
static bool		_debug(Q_LISTTBL *tbl, FILE *out);

static void		_lock(Q_LISTTBL *tbl);
static void		_unlock(Q_LISTTBL *tbl);

static void		_free(Q_LISTTBL *tbl);

/* internal functions */
static bool		_put2(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique, bool first);
static void*		_get2(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem, bool backward, int (*cmp)(const char *s1, const char *s2));

#endif

/**
 * Create a new Q_LIST linked-list container
 *
 * @return	a pointer of malloced Q_LISTTBL structure in case of successful, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   Q_LISTTBL *tbl = qListtbl();
 * @endcode
 */
Q_LISTTBL *qListtbl(void) {
	Q_LISTTBL *tbl = (Q_LISTTBL *)malloc(sizeof(Q_LISTTBL));
	if(tbl == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	// initialize container
	memset((void *)tbl, 0, sizeof(Q_LISTTBL));

	// member methods
	tbl->setPutDirection	= _setPutDirection;

	tbl->put		= _put;
	tbl->putFirst		= _putFirst;
	tbl->putLast		= _putLast;

	tbl->putStr		= _putStr;
	tbl->putStrf		= _putStrf;
	tbl->putInt		= _putInt;

	tbl->get		= _get;
	tbl->getLast		= _getLast;

	tbl->getStr		= _getStr;
	tbl->getInt		= _getInt;

	tbl->getCase		= _getCase;
	tbl->getCaseStr		= _getCaseStr;
	tbl->getCaseInt		= _getCaseInt;

	tbl->getNext		= _getNext;

	tbl->remove		= _remove;

	tbl->size		= _size;
	tbl->reverse		= _reverse;
	tbl->clear		= _clear;

	tbl->parseStr		= _parseStr;
	tbl->save		= _save;
	tbl->load		= _load;
	tbl->debug		= _debug;

	tbl->lock		= _lock;
	tbl->unlock		= _unlock;

	tbl->free		= _free;

	// initialize recrusive mutex
	Q_MUTEX_INIT(tbl->qmutex, true);

	return tbl;
}

/**
 * Q_LISTTBL->setPutDirection(): Sets default adding direction(first or last).
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param first		direction flag. false for adding at the end of this list by default, true for adding at the beginning of this list by default.  (default is false)
 *
 * @return	previous direction.
 *
 * @note
 * The default direction is false which means adding new element at the end of list.
 */
static bool _setPutDirection(Q_LISTTBL *tbl, bool first) {
	bool prevdir = tbl->putdir;
	tbl->putdir = first;

	return prevdir;
}

/**
 * Q_LISTTBL->put(): Put an element to this table.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param data		a pointer which points data memory.
 * @param size		size of the data.
 * @param unique	set true to remove existing elements of same name if exists, false for adding.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   struct my_obj {
 *     int a, b, c;
 *   };
 *
 *   // create a sample object.
 *   struct my_obj obj;
 *
 *   // create a list and add the sample object.
 *   Q_LISTTBL *tbl = qListtbl();
 *   tbl->put(tbl, "obj1", &obj, sizeof(struct my_obj), false);
 * @endcode
 *
 * @note
 * The default behavior is adding object at the end of this table unless it's changed by calling setPutDirection().
 */
static bool _put(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique) {
	return _put2(tbl, name, data, size, unique, tbl->putdir);
}

/**
 * Q_LISTTBL->putFirst(): Put an element at the beginning of this table.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param data		a pointer which points data memory.
 * @param size		size of the data.
 * @param unique	set true to remove existing elements of same name if exists, false for adding.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _putFirst(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique) {
	return _put2(tbl, name, data, size, unique, true);
}

/**
 * Q_LISTTBL->putLast(): Put an object at the end of this table.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param data		a pointer which points data memory.
 * @param size		size of the data.
 * @param unique	set true to remove existing elements of same name if exists, false for adding.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _putLast(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique) {
	return _put2(tbl, name, data, size, unique, false);
}

/**
 * Q_LISTTBL->putStr(): Put a string into this table.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param str		string data.
 * @param unique	set true to remove existing elements of same name if exists, false for adding.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * The default behavior is adding object at the end of this list unless it's changed by calling setDirection().
 */
static bool _putStr(Q_LISTTBL *tbl, const char *name, const char *str, bool unique) {
	size_t size = (str!=NULL) ? (strlen(str) + 1) : 0;
	return _put(tbl, name, (const void *)str, size, unique);
}

/**
 * Q_LISTTBL->putStrf(): Put a formatted string into this table.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param unique	set true to remove existing elements of same name if exists, false for adding.
 * @param name		element name.
 * @param format	formatted value string.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * The default behavior is adding object at the end of this list unless it's changed by calling setDirection().
 */
static bool _putStrf(Q_LISTTBL *tbl, bool unique, const char *name, const char *format, ...) {
	char *str;
	DYNAMIC_VSPRINTF(str, format);
	if(str == NULL) {
		errno = ENOMEM;
		return false;
	}

	bool ret = _putStr(tbl, name, str, unique);
	free(str);

	return ret;
}

/**
 * Q_LISTTBL->putInt(): Put an integer into this table as string type.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param num		number data.
 * @param unique	set true to remove existing elements of same name if exists, false for adding.
 *
 * @return	true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * The integer will be converted to a string object and stored as a string object.
 * The default behavior is adding object at the end of this list unless it's changed by calling setDirection().
 */
static bool _putInt(Q_LISTTBL *tbl, const char *name, int num, bool unique) {
        char str[20+1];
        snprintf(str, sizeof(str), "%d", num);
	return _putStr(tbl, name, str, unique);
}

/**
 * Q_LISTTBL->get(): Finds an object with given name.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param size		if size is not NULL, data size will be stored.
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return	a pointer of data if key is found, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 * 	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   Q_LISTTBL *tbl = qListtbl();
 *   (...codes...)
 *
 *   // with newmem flag unset
 *   size_t size;
 *   void *data = tbl->get(tbl, "key_name", &size, false);
 *
 *   // with newmem flag set
 *   size_t size;
 *   void *data = tbl->get(tbl, "key_name", &size, true);
 *   (...does something with data...)
 *   if(data != NULL) free(data);
 * @endcode
 *
 * @note
 * If newmem flag is set, returned data will be malloced and should be deallocated by user.
 * Otherwise returned pointer will point internal data buffer directly and should not be de-allocated by user.
 * In thread-safe mode, always set newmem flag as true to make sure it works in race condition situation.
 */
static void *_get(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem) {
	return _get2(tbl, name, size, newmem, false, strcmp);
}

/**
 * Q_LISTTBL->getLast(): Finds an object backward from the last of list with given name.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param size		if size is not NULL, data size will be stored.
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return	a pointer of data if key is found, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 * 	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * This can be used for find last matched element if there are multiple objects with same name.
 */
static void *_getLast(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem) {
	return _get2(tbl, name, size, newmem, true, strcmp);
}

/**
 * Q_LISTTBL->getStr():  Finds an object with given name and returns as string type.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return	a pointer of data if key is found, otherwise returns NULL.
  * @retval	errno	will be set in error condition.
 * 	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
*/
static char *_getStr(Q_LISTTBL *tbl, const char *name, bool newmem) {
	return (char *)_get(tbl, name, NULL, newmem);
}

/**
 * Q_LISTTBL->getInt():  Finds an object with given name and returns as integer type.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 *
 * @return	an integer value of the integer object, otherwise returns 0.
 * @retval	errno	will be set in error condition.
 * 	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static int _getInt(Q_LISTTBL *tbl, const char *name) {
	int num = 0;
	char *str = _getStr(tbl, name, true);
	if(str != NULL) {
		num = atoi(str);
		free(str);
	}
	return num;
}

/**
 * Q_LISTTBL->getCase(): Finds an object with given name. (case-insensitive)
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param size		if size is not NULL, data size will be stored.
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return	a pointer of data if key is found, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 * 	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static void *_getCase(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem) {
	return _get2(tbl, name, size, newmem, false, strcasecmp);
}

/**
 * Q_LISTTBL->getCaseStr(): same as getStr() but case-insensitive.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 * @param newmem	whether or not to allocate memory for the data.
 *
 * @return	a pointer of data if key is found, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 * 	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static char *_getCaseStr(Q_LISTTBL *tbl, const char *name, bool newmem) {
	return (char *)_getCase(tbl, name, NULL, newmem);
}

/**
 * Q_LISTTBL->getCaseInt(): same as getInt() but case-insensitive.
 *
 * @param tbl	Q_LISTTBL container pointer.
 * @param name	element name.
 *
 * @return	an integer value of the integer object, otherwise returns 0.
 * @retval	errno	will be set in error condition.
 * 	- ENOENT	: No such key found.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static int _getCaseInt(Q_LISTTBL *tbl, const char *name) {
	int num = 0;
	char *str = _getCase(tbl, name, NULL, true);
	if(str != NULL) {
		num = atoi(str);
		free(str);
	}
	return num;
}

/**
 * Q_LISTTBL->getNext(): Get next element.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param obj		found data will be stored in this object
 * @param name		element name., if key name is NULL search every objects in the list.
 * @param newmem	whether or not to allocate memory for the data.
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
 *   Q_LISTTBL *tbl = qListtbl();
 *   (...add data into list...)
 *
 *   // non-thread usages
 *   Q_NDLOBJ_T obj;
 *   memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *   while(tbl->getNext(tbl, &obj, NULL, false) == true) {
 *     printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char*)obj.data, obj.size);
 *   }
 *
 *   // thread model with particular key search
 *   Q_NDLOBJ_T obj;
 *   memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *   tbl->lock(tbl);
 *   while(tbl->getNext(tbl, &obj, "key_name", true) == true) {
 *     printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char*)obj.data, obj.size);
 *     free(obj.name);
 *     free(obj.data);
 *   }
 *   tbl->unlock(tbl);
 * @endcode
 */
static bool _getNext(Q_LISTTBL *tbl, Q_NDLOBJ_T *obj, const char *name, bool newmem) {
	if(obj == NULL) return NULL;

	_lock(tbl);

	Q_NDLOBJ_T *cont = NULL;

	if(obj->prev == NULL && obj->next == NULL) cont = tbl->first;
	else cont = obj->next;

	if(cont == NULL) {
		errno = ENOENT;
		_unlock(tbl);
		return false;
	}

	bool ret = false;
	for(; cont != NULL; cont = cont->next) {
		if(name == NULL || !strcmp(cont->name, name)) {
			if(newmem == true) {
				obj->name = strdup(cont->name);
				obj->data = malloc(cont->size);
				if(obj->name == NULL || obj->data == NULL) {
					if(obj->name != NULL) free(obj->name);
					if(obj->data != NULL) free(obj->data);
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

	_unlock(tbl);
	return ret;
}

/**
 * Q_LISTTBL->remove(): Remove matched objects as given name.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param name		element name.
 *
 * @return	a number of removed objects.
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: No such key found.
 */
static bool _remove(Q_LISTTBL *tbl, const char *name) {
	if(name == NULL) return false;

	_lock(tbl);

	bool removed = false;
	Q_NDLOBJ_T *obj;
	for (obj = tbl->first; obj != NULL;) {
		if (!strcmp(obj->name, name)) { // found
			// copy next chain
			Q_NDLOBJ_T *prev = obj->prev;
			Q_NDLOBJ_T *next = obj->next;

			// adjust counter
			tbl->num--;
			removed = true;

			// remove list itself
			free(obj->name);
			free(obj->data);
			free(obj);

			// adjust chain links
			if(next == NULL) tbl->last = prev;	// if the object is last one
			if(prev == NULL) tbl->first = next;	// if the object is first one
			else {					// if the object is middle or last one
				prev->next = next;
				next->prev = prev;
			}

			// set next list
			obj = next;
		} else {
			// set next list
			obj = obj->next;
		}
	}

	_unlock(tbl);

	if(removed == false) errno = ENOENT;
	return removed;
}

/**
 * Q_LISTTBL->size(): Returns the number of elements in this list.
 *
 * @param tbl		Q_LISTTBL container pointer.
 *
 * @return	the number of elements in this list.
 */
static size_t _size(Q_LISTTBL *tbl) {
	return tbl->num;
}

/**
 * Q_LISTTBL->reverse(): Reverse the order of elements.
 *
 * @param tbl		Q_LISTTBL container pointer.
 *
 * @return	true if successful otherwise returns false.
 */
static void _reverse(Q_LISTTBL *tbl) {
	_lock(tbl);
	Q_NDLOBJ_T *obj;
	for (obj = tbl->first; obj;) {
		Q_NDLOBJ_T *next = obj->next;
		obj->next = obj->prev;
		obj->prev = next;
		obj = next;
	}

	obj = tbl->first;
	tbl->first = tbl->last;
	tbl->last = obj;

	_unlock(tbl);
}

/**
 * Q_LISTTBL->clear(): Removes all of the elements from this list.
 *
 * @param tbl		Q_LISTTBL container pointer.
 */
static void _clear(Q_LISTTBL *tbl) {
	_lock(tbl);
	Q_NDLOBJ_T *obj;
	for(obj = tbl->first; obj;) {
		Q_NDLOBJ_T *next = obj->next;
		free(obj->name);
		free(obj->data);
		free(obj);
		obj = next;
	}

	tbl->num = 0;
	tbl->first = NULL;
	tbl->last = NULL;
	_unlock(tbl);
}

/**
 * Q_LISTTBL->parseStr(): Parse a string and replace variables in the string to the data in this list.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param str		string value which may contain variables like ${...}
 *
 * @return	malloced string if successful, otherwise returns NULL.
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *
 * @code
 *   ${key_name}          - replace this with a matched value data in this list.
 *   ${!system_command}   - run external command and put it's output here.
 *   ${%PATH}             - get environment variable.
 * @endcode
 *
 * @code
 *   --[tbl Table]------------------------
 *   NAME = qLibc
 *   -------------------------------------
 *
 *   char *str = tbl->parseStr(tbl, "${NAME}, ${%HOME}, ${!date -u}");
 *   if(info != NULL) {
 *     printf("%s\n", str);
 *     free(str);
 *   }
 *
 *  [Output]
 *  qLibc, /home/qlibc, Wed Nov 24 00:30:58 UTC 2010
 *
 * @endcode
 */
static char *_parseStr(Q_LISTTBL *tbl, const char *str) {
	if(str == NULL) {
		errno = EINVAL;
		return NULL;
	}

	_lock(tbl);

	bool loop;
	char *value = strdup(str);
	do {
		loop = false;

		// find ${
		char *s, *e;
		int openedbrakets;
		for(s = value; *s != '\0'; s++) {
			if(!(*s == _VAR && *(s+1) == _VAR_OPEN)) continue;

			// found ${, try to find }. s points $
			openedbrakets = 1; // braket open counter
			for(e = s + 2; *e != '\0'; e++) {
				if(*e == _VAR && *(e+1) == _VAR_OPEN) { // found internal ${
					s = e - 1; // e is always bigger than s, so negative overflow never occured
					break;
				}
				else if(*e == _VAR_OPEN) openedbrakets++;
				else if(*e == _VAR_CLOSE) openedbrakets--;
				else continue;

				if(openedbrakets == 0) break;
			}
			if(*e == '\0') break; // braket mismatch
			if(openedbrakets > 0) continue; // found internal ${

			// pick string between ${, }
			int varlen = e - s - 2; // length between ${ , }
			char *varstr = (char*)malloc(varlen + 3 + 1);
			if(varstr == NULL) continue;
			strncpy(varstr, s + 2, varlen);
			varstr[varlen] = '\0';

			// get the new string to replace
			char *newstr = NULL;
			switch (varstr[0]) {
				case _VAR_CMD : {
					if ((newstr = qStrTrim(qSysCmd(varstr + 1))) == NULL) newstr = strdup("");
					break;
				}
				case _VAR_ENV : {
					newstr = strdup(qSysGetEnv(varstr + 1, ""));
					break;
				}
				default : {
					if ((newstr = _getStr(tbl, varstr, true)) == NULL) {
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
	} while(loop == true);

	_unlock(tbl);

	return value;
}

/**
 * Q_LISTTBL->save(): Save Q_LISTTBL as plain text format
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param filepath	save file path
 * @param sepchar	separator character between name and value. normally '=' is used.
 * @param encode	flag for encoding data . false can be used if the all data are string or integer type and has no new line. otherwise true must be set.
 *
 * @return	true if successful, otherwise returns false.
 */
static bool _save(Q_LISTTBL *tbl, const char *filepath, char sepchar, bool encode) {
	int fd;
	if ((fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, DEF_FILE_MODE)) < 0) {
		DEBUG("Q_LISTTBL->save(): Can't open file %s", filepath);
		return false;
	}

	char *gmtstr = qTimeGetGmtStr(0);
	qIoPrintf(fd, 0, "# Generated by " _Q_PRGNAME " at %s.\n", gmtstr);
	qIoPrintf(fd, 0, "# %s\n", filepath);
	free(gmtstr);

	_lock(tbl);
	Q_NDLOBJ_T *obj;
	for(obj = tbl->first; obj; obj = obj->next) {
		char *encval;
		if(encode == true) encval = qUrlEncode(obj->data, obj->size);
		else encval = obj->data;
		qIoPrintf(fd, 0, "%s%c%s\n", obj->name, sepchar, encval);
		if(encode == true) free(encval);
	}
	_unlock(tbl);

	close(fd);
	return true;
}

/**
 * Q_LISTTBL->load(): Load and append entries from given filepath
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param filepath	save file path
 * @param sepchar	separator character between name and value. normally '=' is used
 * @param decode	flag for decoding data
 *
 * @return	a number of loaded entries.
 */
static size_t _load(Q_LISTTBL *tbl, const char *filepath, char sepchar, bool decode) {
	Q_LISTTBL *loaded;
	if ((loaded = qConfigParseFile(NULL, filepath, sepchar)) == NULL) return false;

	_lock(tbl);
	size_t cnt;
	Q_NDLOBJ_T *obj;
	for(cnt = 0, obj = loaded->first; obj; obj = obj->next) {
		if(decode == true) obj->size = qUrlDecode(obj->data);
		_put(tbl, obj->name, obj->data, obj->size, false);
		cnt++;
	}

	_free(loaded);
	_unlock(tbl);

	return cnt;
}

/**
 * Q_LISTTBL->debug(): Print out stored elements for debugging purpose.
 *
 * @param tbl		Q_LISTTBL container pointer.
 * @param out		output stream FILE descriptor such like stdout, stderr.
 *
 * @return		true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EIO		: Invalid output stream.
 */
static bool _debug(Q_LISTTBL *tbl, FILE *out) {
	if(out == NULL) {
		errno = EIO;
		return false;
	}

	_lock(tbl);
	Q_NDLOBJ_T *obj;
	for(obj = tbl->first; obj; obj = obj->next) {
		fprintf(out, "%s=" , obj->name);
		_q_humanOut(out, obj->data, obj->size, MAX_HUMANOUT);
		fprintf(out, " (%zu)\n" , obj->size);
	}
	_unlock(tbl);

	return true;
}

/**
 * Q_LISTTBL->lock(): Enter critical section.
 *
 * @param tbl		Q_LISTTBL container pointer.
 *
 * @note
 * From user side, normally locking operation is only needed when traverse all elements using Q_LISTTBL->getNext().
 * Most of other operations do necessary locking internally when it's compiled with --enable-threadsafe option.
 *
 * @note
 * This operation will be activated only when --enable-threadsafe option is given at compile time.
 * To find out whether it's compiled with threadsafe option, call qLibcThreadsafe().
 */
static void _lock(Q_LISTTBL *tbl) {
	Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * Q_LISTTBL->unlock(): Leave critical section.
 *
 * @param tbl		Q_LISTTBL container pointer.
 *
 * @note
 * This operation will be activated only when --enable-threadsafe option is given at compile time.
 * To find out whether it's compiled with threadsafe option, call qLibcThreadsafe().
 */
static void _unlock(Q_LISTTBL *tbl) {
	Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * Q_LISTTBL->free(): Free Q_LISTTBL
 *
 * @param tbl		Q_LISTTBL container pointer.
 */
static void _free(Q_LISTTBL *tbl) {
	_clear(tbl);
	Q_MUTEX_DESTROY(tbl->qmutex);

	free(tbl);
}

#ifndef _DOXYGEN_SKIP

static bool _put2(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool unique, bool first) {
	// check arguments
	if(name == NULL || data == NULL || size <= 0) {
		errno = EINVAL;
		return false;
	}

	// duplicate name. name can be NULL
	char *dup_name = strdup(name);
	if(dup_name == NULL) {
		errno = ENOMEM;
		return false;
	}

	// duplicate object
	void *dup_data = malloc(size);
	if(dup_data == NULL) {
		free(dup_name);
		errno = ENOMEM;
		return false;
	}
	memcpy(dup_data, data, size);

	// make new object list
	Q_NDLOBJ_T *obj = (Q_NDLOBJ_T*)malloc(sizeof(Q_NDLOBJ_T));
	if(obj == NULL) {
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

	_lock(tbl);

	// if unique flag is set, remove same key
	if (unique == true) _remove(tbl, dup_name);

	// make link
	if(tbl->num == 0) {
		tbl->first = tbl->last = obj;
	} else {
		if(first == true) {
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

	_unlock(tbl);

	return true;
}

static void *_get2(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem, bool backward, int (*cmp)(const char *s1, const char *s2)) {
	if(name == NULL) {
		errno = EINVAL;
		return NULL;
	}

	_lock(tbl);
	void *data = NULL;
	Q_NDLOBJ_T *obj;

	if(backward == false) obj = tbl->first;
	else obj = tbl->last;

	while(obj != NULL) {
		if(!cmp(obj->name, name)) {
			if(newmem == true) {
				data = malloc(obj->size);
				if(data == NULL) {
					errno = ENOMEM;
					return false;
				}
				memcpy(data, obj->data, obj->size);
			} else {
				data = obj->data;
			}
			if(size != NULL) *size = obj->size;

			break;
		}

		if(backward == false) obj = obj->next;
		else obj = obj->prev;
	}
	_unlock(tbl);

	if(data == NULL) {
		errno = ENOENT;
	}

	return data;
}

#endif /* _DOXYGEN_SKIP */
