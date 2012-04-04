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
    // initialize hash-table
    char memory[1000 * 10];
    qhasharr_t *tbl = qhasharr(memory, sizeof(memory));
    if (tbl == NULL) {
        return -1;
    }

    //
    // TEST 1 : adding elements.
    //

    // insert elements (key duplication is not allowed)
    tbl->put_str(tbl, "e1", "a");
    tbl->put_str(tbl, "e2", "b");
    tbl->put_str(tbl, "e2", "c");
    tbl->put_str(tbl, "e3", "d");
    tbl->put_str(tbl, "e4", "e");
    tbl->put_str(tbl, "e5", "f");
    tbl->put_str(tbl, "12345678901234567890", "1234567890123456789012345678901234567890");

    // print out
    printf("--[Test 1 : adding elements]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 2 : many ways to find key.
    //

    printf("\n--[Test 2 : many ways to find key]--\n");
    char *e2 = tbl->get_str(tbl, "e2");
    if (e2 != NULL) {
        printf("get_str('e2') : %s\n", e2);
        free(e2);
    }

    //
    // TEST 3 : travesal table.
    //

    printf("\n--[Test 3 : travesal table]--\n");
    printf("table size : %d elements\n", tbl->size(tbl, NULL, NULL));
    int idx = 0;
    qnobj_t obj;
    while (tbl->get_next(tbl, &obj, &idx) == true) {
        printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char *)obj.data, obj.size);
        free(obj.name);
        free(obj.data);
    }

    return 0;
}
