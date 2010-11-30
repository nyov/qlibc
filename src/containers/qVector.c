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
 * @file qVector.c Vector implementation.
 *
 * Q_VECTOR container is a vector implementation. It implements a growable array of objects
 * and it extends container Q_LIST that allow a linked-list to be treated as a vector.
 *
 * @code
 *   [Code sample - Object]
 *   Q_VECTOR *vector = qVector();
 *
 *   // put elements
 *   vector->putStr(vector, "AB");      // no need to supply size
 *   vector->putStrf(vector, "%d", 12); // for formatted string
 *   vector->putStr(vector, "CD");
 *
 *   // get the chunk as a string
 *   char *final = vector->toString(vector);
 *
 *   // print out
 *   printf("Number of elements = %zu\n", vector->size(vector));
 *   printf("Final string = %s\n", final);
 *
 *   // release
 *   free(final);
 *   vector->free(vector);
 *
 *   [Result]
 *   Number of elements = 3
 *   Final string = AB12CD
 * @endcode
 *
 * @code
 *   [Code sample - Object]
 *   // sample object
 *   struct sampleobj {
 *     int num;
 *     char str[10];
 *   };
 *
 *   // get new vector
 *   Q_VECTOR *vector = qVector();
 *
 *   // put objects
 *   int i;
 *   struct sampleobj obj;
 *   for(i = 0; i < 3; i++) {
 *     // filling object with sample data
 *     obj.num  = i;
 *     sprintf(obj.str, "hello%d", i);
 *
 *     // stack
 *     vector->put(vector, (void *)&obj, sizeof(struct sampleobj));
 *   }
 *
 *   // final
 *   struct sampleobj *final = (struct sampleobj *)vector->toArray(vector, NULL);
 *
 *   // print out
 *   printf("Number of Objects = %zu\n", vector->size(vector));
 *   for(i = 0; i < vector->size(vector); i++) {
 *     printf("Object%d %d, %s\n", i+1, final[i].num, final[i].str);
 *   }
 *
 *   // release
 *   free(final);
 *   vector->free(vector);
 *
 *   [Result]
 *   Number of Objects = 3
 *   Object1 0, hello0
 *   Object2 1, hello1
 *   Object3 2, hello2
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

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP
static bool _put(Q_VECTOR *vector, const void *object, size_t size);
static bool _putStr(Q_VECTOR *vector, const char *str);
static bool _putStrf(Q_VECTOR *vector, const char *format, ...);
static void *_toArray(Q_VECTOR *vector, size_t *size);
static void *_toString(Q_VECTOR *vector);
static size_t _size(Q_VECTOR *vector);
static void _clear(Q_VECTOR *vector);
static bool _debug(Q_VECTOR *vector, FILE *out);
static void _free(Q_VECTOR *vector);
#endif

/**
 * Initialize vector.
 *
 * @return		Q_VECTOR container pointer.
 * @retval	errno	will be set in error condition.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @code
 *   // allocate memory
 *   Q_VECTOR *vector = qVector();
 *   vector->free(vector);
 * @endcode
 */
Q_VECTOR *qVector(void) {
	Q_VECTOR *vector = (Q_VECTOR *)malloc(sizeof(Q_VECTOR));
	if(vector == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	memset((void *)vector, 0, sizeof(Q_VECTOR));
	vector->list = qList();
	if(vector->list == NULL) {
		free(vector);
		errno = ENOMEM;
		return NULL;
	}

	// methods
	vector->put		= _put;
	vector->putStr		= _putStr;
	vector->putStrf		= _putStrf;

	vector->toArray		= _toArray;
	vector->toString	= _toString;

	vector->size		= _size;
	vector->clear		= _clear;
	vector->debug		= _debug;
	vector->free		= _free;

	return vector;
}

/**
 * Q_VECTOR->put(): Stack object
 *
 * @param vector	Q_VECTOR container pointer.
 * @param object	a pointer of object data
 * @param size		size of object
 *
 * @return		true if successful, otherwise returns false
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _put(Q_VECTOR *vector, const void *data, size_t size) {
	return vector->list->addLast(vector->list, data, size);
}

/**
 * Q_VECTOR->putStr(): Stack string
 *
 * @param vector	Q_VECTOR container pointer.
 * @param str		a pointer of string
 *
 * @return		true if successful, otherwise returns false
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _putStr(Q_VECTOR *vector, const char *str) {
	return vector->list->addLast(vector->list, str, strlen(str));
}

/**
 * Q_VECTOR->putStrf(): Stack formatted string
 *
 * @param vector	Q_VECTOR container pointer.
 * @param format	string format
 *
 * @return		true if successful, otherwise returns false
 * @retval	errno	will be set in error condition.
 *	- EINVAL	: Invalid argument.
 *	- ENOMEM	: Memory allocation failed.
 */
static bool _putStrf(Q_VECTOR *vector, const char *format, ...) {
	char *str;
	DYNAMIC_VSPRINTF(str, format);
	if(str == NULL) {
		errno = ENOMEM;
		return false;
	}

	bool ret = _putStr(vector, str);
	free(str);

	return ret;
}

/**
 * Q_VECTOR->toArray(): Returns the serialized chunk containing all the elements in this vector.
 *
 * @param vector	Q_VECTOR container pointer.
 * @param size		if size is not NULL, merged object size will be stored.
 *
 * @return	a pointer of finally merged elements(malloced), otherwise returns NULL
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 */
static void *_toArray(Q_VECTOR *vector, size_t *size) {
	return vector->list->toArray(vector->list, size);
}

/**
 * Q_LIST->toString(): Returns a string representation of this list, containing string representation of each element.
 *
 * @param vector	Q_VECTOR container pointer.
 *
 * @return	a pointer of finally merged strings(malloced), otherwise returns NULL
 * @retval	errno	will be set in error condition.
 *	- ENOENT	: List is empty.
 *	- ENOMEM	: Memory allocation failed.
 *
 * @note
 * Return string is always terminated by '\0'.
 */
 static void *_toString(Q_VECTOR *vector) {
	return vector->list->toString(vector->list);
}

/**
 * Q_VECTOR->size(): Returns the number of elements in this list.
 *
 * @param vector	Q_VECTOR container pointer.
 *
 * @return		the number of elements in this vector.
 */
static size_t _size(Q_VECTOR *vector) {
	return vector->list->size(vector->list);
}

/**
 * Q_VECTOR->clear(): Removes all of the elements from this vector.
 *
 * @param vector	Q_VECTOR container pointer.
 */
static void _clear(Q_VECTOR *vector) {
	vector->list->clear(vector->list);
}

/**
 * Q_VECTOR->debug(): Print out stored elements for debugging purpose.
 *
 * @param list		Q_LIST container pointer.
 * @param out		output stream FILE descriptor such like stdout, stderr.
 *
 * @return		true if successful, otherwise returns false.
 * @retval	errno	will be set in error condition.
 *	- EIO		: Invalid output stream.
 */
static bool _debug(Q_VECTOR *vector, FILE *out) {
	return vector->list->debug(vector->list, out);
}

/**
 * Q_VECTOR->free(): De-allocate vector
 *
 * @param vector	Q_VECTOR container pointer.
 */
static void _free(Q_VECTOR *vector) {
	vector->list->free(vector->list);
	free(vector);
}
