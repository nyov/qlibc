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
 * @file qaconf.c Apache-style configuration file parser.
 */

#ifndef DISABLE_QACONF

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "qlibc.h"
#include "qlibcext.h"
#include "qinternal.h"

#define _INCLUDE_DIRECTIVE  "Include "

#ifndef _DOXYGEN_SKIP
#define _VAR        '$'
#define _VAR_OPEN   '{'
#define _VAR_CLOSE  '}'
#define _VAR_CMD    '!'
#define _VAR_ENV    '%'

/* internal functions */
static int addoptions(qaconf_t *qaconf, const qaconf_opt_t *options);
#endif

qaconf_t *qaconf(const char *filepath, uint8_t flags)
{
    // Check arguments
    if (filepath == NULL) return NULL;

    // Malloc qaconf_t structure
    qaconf_t *qaconf = (qaconf_t *)malloc(sizeof(qaconf_t));
    if (qaconf == NULL) return NULL;

    // Initialize the structure
    memset((void *)(qaconf), '\0', sizeof(qaconf_t));
    qaconf->filepath = strdup(filepath);
    ASSERT(qaconf->filepath != NULL);
    qaconf->flags = flags;

    // member methods
    qaconf->addoptions = addoptions;

    return qaconf;
}

static int addoptions(qaconf_t *qaconf, const qaconf_opt_t *options)
{
    if (qaconf == NULL || options == NULL) return -1;

    // Count a number of options
    uint32_t numopts;
    for (numopts = 0; options[numopts].name != NULL; numopts++);
    if (numopts == 0) return 0;

    // Realloc
    size_t newsize = sizeof(qaconf_opt_t) * (qaconf->numoptions + numopts);
    qaconf->options = (qaconf_opt_t *)realloc(qaconf->options, newsize);
    memcpy(&qaconf->options[qaconf->numoptions], options, sizeof(qaconf_opt_t) * numopts);
    qaconf->numoptions += numopts;

    return numopts;
}

#endif /* DISABLE_QACONF */

