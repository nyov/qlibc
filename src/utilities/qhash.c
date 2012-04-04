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
 * @file qhash.c Hash APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "md5/md5.h"
#include "qlibc.h"
#include "qinternal.h"

/**
 * Get MD5 hash.
 *
 * @param data      source object
 * @param nbytes    size of data
 *
 * @return 128-bit(16-byte) malloced digest binary data
 *
 * @code
 *   unsigned char *md5 = qhash_md5((void*)"hello", 5);
 *   free(md5);
 * @endcode
 */
unsigned char *qhash_md5(const void *data, size_t nbytes)
{
    if (data == NULL) return NULL;

    unsigned char *digest = (unsigned char *)malloc(sizeof(char) * 16);
    if (digest == NULL) return NULL;

    MD5_CTX context;
    MD5Init(&context);
    MD5Update(&context, (unsigned char *)data, (unsigned int)nbytes);
    MD5Final(digest, &context);

    return digest;
}

/**
 * Get MD5 hash string of a object data represented as a sequence of 32
 * hexadecimal digits.
 *
 * @param data      source object
 * @param nbytes    size of data
 *
 * @return 32 bytes(128 bits) long malloced digest binary data malloced 32+1
 *         bytes digested ASCII string which is terminated by NULL character.
 *
 * @code
 *   char *md5str = qhash_md5_str((void*)"hello", 5);
 *   printf("%s\n", md5str);
 *   free(md5str);
 * @endcode
 */
char *qhash_md5_str(const void *data, size_t nbytes)
{
    if (data == NULL) return NULL;

    unsigned char *digest = qhash_md5(data, nbytes);
    if (digest == NULL) return NULL;

    // hexadecimal encoding
    char *md5hex = qhex_encode(digest, 16);
    free(digest);

    return md5hex;
}

/**
 * Get MD5 hash string of a file contents represented as a sequence of 32
 * hexadecimal digits.
 *
 * @param filepath  file path
 * @param nbytes    size of data. Set to NULL to digest end of file
 *
 * @return malloced 33(16*2+1) bytes digested ASCII string which is terminated
 *         by NULL character
 *
 * @note
 *  If the nbytes is set, qhash_md5_file() will try to digest at lease nbytes
 *  then store actual digested size into nbytes. So if you set nbytes to over
 *  size than file size, finally nbytes will have actual file size.
 *
 * @code
 *   // case of digesting entire file
 *   char *md5str = qhash_md5_file("/tmp/test.dat, NULL);
 *   printf("%s\n", md5str);
 *   free(md5str);
 *
 *   // case of nbytes is set to 1 bytes length
 *   size_t nbytes = 1;
 *   char *md5str = qhash_md5_file("/tmp/test.dat, &nbytes);
 *   printf("%s %d\n", md5str, nbytes);
 *   free(md5str);
 *
 *   // case of nbytes is set to over size
 *   size_t nbytes = 100000;
 *   char *md5str = qhash_md5_file("/tmp/test.dat, &nbytes);
 *   printf("%s %d\n", md5str, nbytes);
 *   free(md5str);
 * @endcode
 */
char *qhash_md5_file(const char *filepath, size_t *nbytes)
{
    int fd = open(filepath, O_RDONLY, 0);
    if (fd < 0) return NULL;

    struct stat st;
    if (fstat(fd, &st) < 0) return NULL;

    size_t size = st.st_size;
    if (nbytes != NULL) {
        if (*nbytes > size) *nbytes = size;
        else size = *nbytes;
    }

    MD5_CTX context;
    MD5Init(&context);
    ssize_t retr = 0;
    unsigned char buf[32*1024], szDigest[16];
    while (size > 0) {
        if (size > sizeof(buf)) retr = read(fd, buf, sizeof(buf));
        else retr = read(fd, buf, size);
        if (retr < 0) break;
        MD5Update(&context, buf, retr);
        size -= retr;
    }
    close(fd);
    MD5Final(szDigest, &context);

    if (nbytes != NULL) *nbytes -= size;

    // hexadecimal encoding
    char *md5hex = qhex_encode(szDigest, 16);
    return md5hex;
}

/**
 * Get 32-bit FNV hash integer.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 32-bit hash integer
 *
 * @code
 *   uint32_t fnv32 = qhash_fnv32((void*)"hello", 5);
 * @endcode
 */
uint32_t qhash_fnv32(const void *data, size_t nbytes)
{
    if (data == NULL) return 0;

    unsigned char *dp;
    uint32_t hval = 0x811C9DC5;

    for (dp = (unsigned char *)data; *dp && nbytes > 0; dp++, nbytes--) {
        hval *= 0x01000193;
        hval ^= *dp;
    }

    return hval;
}

/**
 * Get 64-bit FNV hash integer.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 64-bit hash integer
 *
 * @code
 *   uint64_t fnv64 = qhash_fnv64((void*)"hello", 5);
 * @endcode
 */
uint64_t qhash_fnv64(const void *data, size_t nbytes)
{
    if (data == NULL) return 0;

    unsigned char *dp;
    uint64_t hval = 0xCBF29CE484222325ULL;

    for (dp = (unsigned char *)data; *dp && nbytes > 0; dp++, nbytes--) {
        hval *= 0x100000001B3ULL;
        hval ^= *dp;
    }

    return hval;
}
