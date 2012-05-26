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

#include "qlibc.h"
#include "qlibcext.h"

#ifdef DISABLE_QACONF
int main(void)
{
    printf("qaconf extension is disabled at compile time.\n");
    return 1;
}
#else

// Configuration file to parse.
#define CONF_PATH   "apacheconf.conf"

// Define scope.
//   QACONF_SCOPE_ALL and QACONF_SCOPE_ROOT are predefined.
//   Custum scop should be defined from 2(1 << 1).
//   Note) These values are ORed(bit operation), so the number should be
//         2(1<<1), 4(1<<2), 6(1<<3), 8(1<<4), ...
enum {
    OPT_WHERE_ALL        = QACONF_SCOPE_ALL,   /* pre-defined */
    OPT_WHERE_ROOT       = QACONF_SCOPE_ROOT,  /* pre-defined */
    OPT_WHERE_NODES      = (1 << 1),    /* user-defined scope */
    OPT_WHERE_PARTITIONS = (1 << 2),    /* user-defined scope */
};

// Define callback proto-types.
static QACONF_CB(confcb_listen);

// Define options.
static qaconf_opt_t options[] = {
    {"Listen", confcb_listen, 0, OPT_WHERE_ALL},
    {"CacheLookupHint", confcb_listen, 0, OPT_WHERE_ALL},
    {"LookupCacheSize", confcb_listen, 0, OPT_WHERE_ALL},
    {"Listen", confcb_listen, 0, OPT_WHERE_ALL},
    {NULL, NULL, 0, 0}
};

int main(void)
{
    // Initialize and create a qaconf object.
    qaconf_t *conf = qaconf(CONF_PATH, 0);
    if (conf == NULL) {
        printf("Failed to open '" CONF_PATH "'.\n");
        return -1;
    }

    // Register options;
    conf->addoptions

    return 0;
}

static QACONF_CB(confcb_listen)
{
    return NULL;
}

#endif
