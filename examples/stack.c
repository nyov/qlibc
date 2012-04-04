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
    // create stack
    qstack_t *stack = qstack();

    // example: integer stack
    stack->push_int(stack, 1);
    stack->push_int(stack, 2);
    stack->push_int(stack, 3);

    printf("pop_int(): %d\n", stack->pop_int(stack));
    printf("pop_int(): %d\n", stack->pop_int(stack));
    printf("pop_int(): %d\n", stack->pop_int(stack));

    // example: string stack
    stack->push_str(stack, "A string");
    stack->push_str(stack, "B string");
    stack->push_str(stack, "C string");

    char *str = stack->pop_str(stack);
    printf("pop_str(): %s\n", str);
    free(str);
    str = stack->pop_str(stack);
    printf("pop_str(): %s\n", str);
    free(str);
    str = stack->pop_str(stack);
    printf("pop_str(): %s\n", str);
    free(str);

    // example: object stack
    stack->push(stack, "A object", sizeof("A object"));
    stack->push(stack, "B object", sizeof("B object"));
    stack->push(stack, "C object", sizeof("C object"));

    void *obj = stack->pop(stack, NULL);
    printf("pop(): %s\n", (char *)obj);
    free(obj);
    obj = stack->pop(stack, NULL);
    printf("pop(): %s\n", (char *)obj);
    free(obj);
    obj = stack->pop(stack, NULL);
    printf("pop(): %s\n", (char *)obj);
    free(obj);

    // release
    stack->free(stack);

    return 0;
}
