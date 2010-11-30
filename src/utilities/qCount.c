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
 * @file qCount.c Counter file handling APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "qlibc.h"
#include "qInternal.h"

/**
 * Read counter(integer) from file with advisory file locking.
 *
 * @param filepath	file path
 *
 * @return		counter value readed from file. in case of failure, returns 0.
 *
 * @code
 *   qCountSave("number.dat", 75);
 *   int count = qCountRead("number.dat");
 * @endcode
 *
 * @code
 *   ---- number.dat ----
 *   75
 *   --------------------
 * @endcode
 */
int qCountRead(const char *filepath) {
	int fd = open(filepath, O_RDONLY, 0);
	if(fd < 0) return 0;

	char buf[10+1];
	ssize_t readed = qIoRead(buf, fd, (sizeof(buf) - 1), 0);
	close(fd);

	if(readed > 0) {
		buf[readed] = '\0';
		return atoi(buf);
	}
	return 0;
}

/**
 * Save counter(integer) to file with advisory file locking.
 *
 * @param filepath	file path
 * @param number	counter integer value
 *
 * @return		true if successful, otherwise returns false.
 *
 * @note
 * @code
 *   qCountSave("number.dat", 75);
 * @endcode
 */
bool qCountSave(const char *filepath, int number) {
	int fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, DEF_FILE_MODE);
	if(fd < 0) return false;

	ssize_t updated = qIoPrintf(fd, 0, "%d", number);
	close(fd);

	if(updated > 0) return true;
	return false;
}

/**
 * Increases(or decrease) the counter value as much as specified number
 * with advisory file locking.
 *
 * @param filepath	file path
 * @param number	how much increase or decrease
 *
 * @return		updated counter value. in case of failure, returns 0.
 *
 * @note
 * @code
 *   int count;
 *   count = qCountUpdate("number.dat", -3);
 * @endcode
 */
int qCountUpdate(const char *filepath, int number) {
	int counter = qCountRead(filepath);
	counter += number;
	if(qCountSave(filepath, counter) == true) return counter;
	return 0;
}
