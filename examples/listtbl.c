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
    // create a list-table.
    qlisttbl_t *tbl = qlisttbl();

    //
    // TEST 1 : adding elements.
    //

    // insert elements (key duplication allowed)
    tbl->putstr(tbl, "e1", "a", false);
    tbl->putstr(tbl, "e2", "b", false);
    tbl->putstr(tbl, "e2", "c", false);
    tbl->putstr(tbl, "e2", "d", false);
    tbl->put(tbl, "e3", "e", strlen("e")+1, false); // equal to add_str();

    // print out
    printf("--[Test 1 : adding elements]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 2 : many ways to find key.
    //

    printf("\n--[Test 2 : many ways to find key]--\n");
    printf("get('e2') : %s\n", (char *)tbl->get(tbl, "e2", NULL, false));
    printf("getstr('e2') : %s\n", tbl->getstr(tbl, "e2", false));

    char *e2 = tbl->getstr(tbl, "e2", true);
    printf("getstr('e2') with newmem parameter: %s\n", e2);
    free(e2);

    //
    // TEST 3 : travesal a list.
    //

    printf("--[Test 3 : travesal a list]--\n");
    printf("list size : %zu elements\n", tbl->size(tbl));
    qdlnobj_t obj;
    memset((void *)&obj, 0, sizeof(obj)); // must be cleared before call
    tbl->lock(tbl);
    while (tbl->getnext(tbl, &obj, NULL, true) == true) {
        printf("NAME=%s, DATA=%s, SIZE=%zu\n",
               obj.name, (char *)obj.data, obj.size);
        free(obj.name);
        free(obj.data);
    }
    tbl->unlock(tbl);

    //
    // TEST 4 :  travesal a particular key 'e2'.
    //

    printf("\n--[Test 4 : travesal a particular key 'e2']--\n");
    memset((void *)&obj, 0, sizeof(obj)); // must be cleared before call
    tbl->lock(tbl);
    while (tbl->getnext(tbl, &obj, "e2", false) == true) {
        printf("NAME=%s, DATA=%s, SIZE=%zu\n",
               obj.name, (char *)obj.data, obj.size);
    }
    tbl->unlock(tbl);

    //
    // TEST 5 : changed put direction and added 'e4' and 'e5' element.
    //
    tbl->setputdir(tbl, true);
    tbl->putstr(tbl, "e4", "f", false);
    tbl->putstr(tbl, "e5", "g", false);

    // print out
    printf("\n--[Test 5 : changed adding direction then"
           " added 'e4' and 'e5' element]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 6 :  add element 'e2' with replace option.
    //
    tbl->putstr(tbl, "e2", "h", true);

    // print out
    printf("\n--[Test 6 : add element 'e2' with replace option]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 7 :  reverse list
    //

    tbl->reverse(tbl);

    // print out
    printf("\n--[Test 7 : reverse]--\n");
    tbl->debug(tbl, stdout);

    // free object
    tbl->free(tbl);

    return 0;
}
