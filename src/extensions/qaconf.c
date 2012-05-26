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
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "qlibc.h"
#include "qlibcext.h"
#include "qinternal.h"

#define _INCLUDE_DIRECTIVE  "Include "

#ifndef _DOXYGEN_SKIP
#define MAX_LINESIZE    (1024*4)

#define _VAR        '$'
#define _VAR_OPEN   '{'
#define _VAR_CLOSE  '}'
#define _VAR_CMD    '!'
#define _VAR_ENV    '%'

/* internal functions */
static int addoptions(qaconf_t *qaconf, const qaconf_option_t *options);
static void setdefhandler(qaconf_t *qaconf, const qaconf_cb_t *callback);
static void setuserdata(qaconf_t *qaconf, const void *userdata);
static int parse(qaconf_t *qaconf, const char *filepath, uint8_t flags);
static const char *errmsg(qaconf_t *qaconf);
static void free_(qaconf_t *qaconf);

static int _parse_inline(qaconf_t *qaconf, FILE *fp, uint8_t flags,
                         int lineno, qaconf_cbdata_t *cbdata_parent);
static void _seterrmsg(qaconf_t *qaconf, const char *format, ...);
static void _free_cbdata(qaconf_cbdata_t *cbdata);
#endif

qaconf_t *qaconf(void)
{
    // Malloc qaconf_t structure
    qaconf_t *qaconf = (qaconf_t *)malloc(sizeof(qaconf_t));
    if (qaconf == NULL) return NULL;

    // Initialize the structure
    memset((void *)(qaconf), '\0', sizeof(qaconf_t));

    // member methods
    qaconf->addoptions      = addoptions;
    qaconf->setdefhandler   = setdefhandler;
    qaconf->setuserdata     = setuserdata;
    qaconf->parse           = parse;
    qaconf->errmsg          = errmsg;
    qaconf->free            = free_;

    return qaconf;
}

static int addoptions(qaconf_t *qaconf, const qaconf_option_t *options)
{
    if (qaconf == NULL || options == NULL) {
        _seterrmsg(qaconf, "Invalid parameters.");
        return -1;
    }

    // Count a number of options
    uint32_t numopts;
    for (numopts = 0; options[numopts].name != NULL; numopts++);
    if (numopts == 0) return 0;

    // Realloc
    size_t newsize = sizeof(qaconf_option_t) * (qaconf->numoptions + numopts);
    qaconf->options = (qaconf_option_t *)realloc(qaconf->options, newsize);
    memcpy(&qaconf->options[qaconf->numoptions], options, sizeof(qaconf_option_t) * numopts);
    qaconf->numoptions += numopts;

    return numopts;
}

static void setdefhandler(qaconf_t *qaconf, const qaconf_cb_t *callback)
{
    qaconf->defcb = callback;
}

static void setuserdata(qaconf_t *qaconf, const void *userdata)
{
    qaconf->userdata = (void *)userdata;
}

static int parse(qaconf_t *qaconf, const char *filepath, uint8_t flags)
{
    // Open file
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        _seterrmsg(qaconf, "Failed to open file '%s'.", filepath);
        return -1;
    }
    // Parse
    int optcount = _parse_inline(qaconf, fp, flags, 0, NULL);

    // Clean up
    fclose(fp);

    return optcount;
}

static const char *errmsg(qaconf_t *qaconf)
{
    return (const char*)qaconf->errstr;
}

static void free_(qaconf_t *qaconf)
{
    if (qaconf->errstr != NULL) free(qaconf->errstr);
    if (qaconf->options != NULL) free(qaconf->options);
    free(qaconf);
}

#ifndef _DOXYGEN_SKIP
#define ARGV_INIT_SIZE  (4)
#define ARGV_INCR_STEP  (8)
static int _parse_inline(qaconf_t *qaconf, FILE *fp, uint8_t flags,
                         int lineno, qaconf_cbdata_t *cbdata_parent)
{
    // Assign compare function.
    int (*cmpfunc)(const char *, const char *) = strcmp;
    if (flags & QAC_CASEINSENSITIVE) cmpfunc = strcasecmp;

    char buf[MAX_LINESIZE];
    bool exception = false;
    int optcount = 0;  // number of option entry processed.
    while (exception == false && fgets(buf, MAX_LINESIZE, fp) != NULL) {
        lineno++;

        // Trim white spaces
        qstrtrim(buf);

        // Skip blank like and comments.
        if (IS_EMPTY_STR(buf) || *buf == '#') {
            continue;
        }

        DEBUG("%s (line=%d)", buf, lineno);

        // Create a callback data
        qaconf_cbdata_t *cbdata = (qaconf_cbdata_t*)malloc(sizeof(qaconf_cbdata_t));
        ASSERT(cbdata != NULL);
        memset(cbdata, '\0', sizeof(qaconf_cbdata_t));
        cbdata->parent = cbdata_parent;

        // Escape scope option
        char *sp = buf;
        if (*sp == '<') {
            if (ENDING_CHAR(sp) != '>') {
                _seterrmsg(qaconf, "Line %d: Missing closing bracket - '%s'.",
                           lineno, buf);
                exception = true;
                _free_cbdata(cbdata);
                break;
            }

            sp++;
            if (*sp == '/') {
                cbdata->otype = QAC_OTYPE_SCOPECLOSE;
                sp++;
            } else {
                cbdata->otype = QAC_OTYPE_SCOPEOPEN;
            }

            // Remove tailing braket
            ENDING_CHAR(sp) = '\0';
        } else {
            cbdata->otype = QAC_OTYPE_OPTION;
        }

        // Copy data into cbdata buffer.
        cbdata->data = strdup(sp);
        ASSERT(cbdata->data != NULL);

        // Parse option line.
        int argvsize = 0;
        char *wp1, *wp2;
        bool done = false;
        for (wp1 = (char *)cbdata->data; done == false; wp1 = wp2) {
            bool dq = false, sq = false;

            // Allocate/Realloc argv array
            if (argvsize == cbdata->argc) {
                argvsize += (argvsize == 0) ? ARGV_INIT_SIZE : ARGV_INCR_STEP;
                cbdata->argv = (char**)realloc((void *)cbdata->argv, sizeof(char*) * argvsize);
                ASSERT(cbdata->argv != NULL);
            }

            // Quote handling
            if (*wp1 == '"') {
                dq = true;
                wp1++;
            } else if (*wp1 == '\'') {
                sq = true;
                wp1++;
            } else if (*wp1 == ' ' || *wp1 == '\t') {
                for (wp1++; (*wp1 == ' ' || *wp1 == '\t'); wp1++);
            }

            // Parse a word
            for (wp2 = wp1; ; wp2++) {
                if (*wp2 == '\0') {
                    done = true;
                    break;
                } else if (*wp1 == '"') {
                    // todo
                    break;
                } else if (*wp1 == '\'') {
                    // todo
                    break;
                } else if (*wp2 == ' ' || *wp2 == '\t') {
                    // todo
                    break;
                }
            }
            *wp2 = '\0';
            wp2++;

            // Store a argument
            cbdata->argv[cbdata->argc] = wp1;
            cbdata->argc++;

            DEBUG("  argv[%d]=%s", cbdata->argc - 1, wp1);
        }

        // Find matching option
        int i;
        char *argv0 = cbdata->argv[0];
        bool optfound = false;
        for (i = 0; i < qaconf->numoptions; i++) {
            qaconf_option_t *option = &qaconf->options[i];
            if (!cmpfunc(argv0, option->name)) {
                optfound = true;

                // Callback
                DEBUG("Callback %s", argv0);
                if (option->cb != NULL) {
                    option->cb(cbdata, qaconf->userdata);
                } else if (qaconf->defcallback != NULL) {
                    qaconf->defcb(cbdata, qaconf->userdata);
                }
            }
        }

        // If not found.
        if (optfound == false) {
            if (qaconf->defcallback != NULL) {
                qaconf->defcb(cbdata, qaconf->userdata);
            } else if (flags & QAC_IGNOREUNKNOWN == 0) {
                _seterrmsg(qaconf, "Line %d: Unregistered option - '%s'.",
                           lineno, buf);
                exception = true;
                _free_cbdata(cbdata);
                break;
            }
        }

        // Release resources
        _free_cbdata(cbdata);
        cbdata = NULL;

        // Go up and down
        // if (otype

        // Increase process counter
        optcount++;
    }

    return (exception == false) ? optcount : -1;
}

static void _seterrmsg(qaconf_t *qaconf, const char *format, ...)
{
    if (qaconf->errstr != NULL) free(qaconf->errstr);
    DYNAMIC_VSPRINTF(qaconf->errstr, format);
}

static void _free_cbdata(qaconf_cbdata_t *cbdata) {
    if (cbdata->argv != NULL) free(cbdata->argv);
    if (cbdata->data != NULL) free(cbdata->data);
    free(cbdata);
}

#endif /* _DOXYGEN_SKIP */

#endif /* DISABLE_QACONF */

