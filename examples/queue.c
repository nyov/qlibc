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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "qlibc.h"

int main(void)
{
    // create queue
    qqueue_t *queue = qqueue();

    // example: integer queue
    queue->push_int(queue, 1);
    queue->push_int(queue, 2);
    queue->push_int(queue, 3);

    printf("pop_int(): %d\n", queue->pop_int(queue));
    printf("pop_int(): %d\n", queue->pop_int(queue));
    printf("pop_int(): %d\n", queue->pop_int(queue));

    // example: string queue
    queue->push_str(queue, "A string");
    queue->push_str(queue, "B string");
    queue->push_str(queue, "C string");

    char *str = queue->pop_str(queue);
    printf("pop_str(): %s\n", str);
    free(str);
    str = queue->pop_str(queue);
    printf("pop_str(): %s\n", str);
    free(str);
    str = queue->pop_str(queue);
    printf("pop_str(): %s\n", str);
    free(str);

    // example: object queue
    queue->push(queue, "A object", sizeof("A object"));
    queue->push(queue, "B object", sizeof("B object"));
    queue->push(queue, "C object", sizeof("C object"));

    void *obj = queue->pop(queue, NULL);
    printf("pop(): %s\n", (char *)obj);
    free(obj);
    obj = queue->pop(queue, NULL);
    printf("pop(): %s\n", (char *)obj);
    free(obj);
    obj = queue->pop(queue, NULL);
    printf("pop(): %s\n", (char *)obj);
    free(obj);

    // release
    queue->free(queue);

    return 0;
}
