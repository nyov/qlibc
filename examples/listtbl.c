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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "qlibc.h"

int main(void) {
	// create a list-table.
	Q_LISTTBL *tbl = qListtbl();

	//
	// TEST 1 : adding elements.
	//

	// insert elements (key duplication allowed)
	tbl->putStr(tbl, "e1", "a", false);
	tbl->putStr(tbl, "e2", "b", false);
	tbl->putStr(tbl, "e2", "c", false);
	tbl->putStr(tbl, "e2", "d", false);
	tbl->put(tbl, "e3", "e", strlen("e3")+1, false); // equal to addStr();

	// print out
	printf("--[Test 1 : adding elements]--\n");
	tbl->debug(tbl, stdout);

	//
	// TEST 2 : many ways to find key.
	//

	printf("\n--[Test 2 : many ways to find key]--\n");
	printf("get('e2') : %s\n", (char*)tbl->get(tbl, "e2", NULL, false));
	printf("getLast('e2') : %s\n", (char*)tbl->getLast(tbl, "e2", NULL, false));
	printf("getStr('e2') : %s\n", tbl->getStr(tbl, "e2", false));

	char *e2 = tbl->getStr(tbl, "e2", true);
	printf("getStr('e2') with newmem parameter: %s\n", e2);
	free(e2);

	//
	// TEST 3 : travesal list.
	//

	printf("--[Test 3 : travesal list]--\n");
	printf("list size : %zu elements\n", tbl->size(tbl));
 	Q_NDLOBJ_T obj;
 	memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 	tbl->lock(tbl);
 	while(tbl->getNext(tbl, &obj, NULL, true) == true) {
		printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char*)obj.data, obj.size);
		free(obj.name);
		free(obj.data);
 	}
 	tbl->unlock(tbl);

	//
	// TEST 4 :  travesal particular key 'e2' only.
	//

	printf("\n--[Test 4 : travesal particular key 'e2' only]--\n");
 	memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 	tbl->lock(tbl);
 	while(tbl->getNext(tbl, &obj, "e2", false) == true) {
 	  printf("NAME=%s, DATA=%s, SIZE=%zu\n", obj.name, (char*)obj.data, obj.size);
 	}
 	tbl->unlock(tbl);

	//
	// TEST 5 : changed put direction and added 'e4' and 'e5' element.
	//
	tbl->setPutDirection(tbl, true);
	tbl->putStr(tbl, "e4", "f", false);
	tbl->putStr(tbl, "e5", "g", false);

	// print out
	printf("\n--[Test 5 : changed adding direction and added 'e4' and 'e5' element]--\n");
	tbl->debug(tbl, stdout);

	//
	// TEST 6 :  add element 'e2' with replace option.
	//
	tbl->putStr(tbl, "e2", "h", true);

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
