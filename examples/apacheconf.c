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

#include <string.h>
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

// User configuration structure.
struct MyConf {
    char *ringid;
    int listen;
};

struct MyConf g_myconf;

// Define scope.
//   QAC_SCOPE_ALL and QAC_SCOPE_ROOT are predefined.
//   Custum scope should be defined from 2(1 << 1).
//   Note) These values are ORed(bit operation), so the number should be
//         2(1<<1), 4(1<<2), 6(1<<3), 8(1<<4), ...
enum {
    OPT_WHERE_ALL        = QAC_SCOPE_ALL,   /* pre-defined */
    OPT_WHERE_ROOT       = QAC_SCOPE_ROOT,  /* pre-defined */
    OPT_WHERE_NODES      = (1 << 1),    /* user-defined scope */
    OPT_WHERE_PARTITIONS = (1 << 2),    /* user-defined scope */
};

// Define callback proto-types.
static QAC_CB(confcb_ringid);
static QAC_CB(confcb_listen);

// Define options.
static qaconf_option_t options[] = {
    {"RingID", QAC_TAKE1_STR, confcb_ringid, 0, OPT_WHERE_ALL},
    {"Listen", QAC_TAKE1_NUM, confcb_listen, 0, OPT_WHERE_ALL},
    QAC_OPTION_END
};

static qaconf_option_t options2[] = {
    {"Node", QAC_TAKE1_NUM, confcb_listen, 0, OPT_WHERE_ALL},
    {"IP", QAC_TAKE1_NUM, confcb_listen, 0, OPT_WHERE_ALL},
    QAC_OPTION_END
};

int main(void)
{
    // Initialize and create a qaconf object.
    qaconf_t *conf = qaconf();
    if (conf == NULL) {
        printf("Failed to open '" CONF_PATH "'.\n");
        return -1;
    }

    // Register options.
    conf->addoptions(conf, options);
    conf->addoptions(conf, options2);
    conf->setuserdata(conf, &g_myconf);

    int count = conf->parse(conf, CONF_PATH, QAC_CASEINSENSITIVE);
    if (count < 0) {
        printf("Error: %s\n", conf->errmsg(conf));
    } else {
        printf("Successfully loaded.\n");
    }

    // Release resources.
    conf->free(conf);

    return 0;
}

static QAC_CB(confcb_ringid)
{
    if (data->argc != 2) {
        return strdup("Invalid argument!!!");
    }

    printf("Hello\n");

    return NULL;
}

static QAC_CB(confcb_listen)
{
    return NULL;
}

#endif
