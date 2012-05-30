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
static void reseterror(qaconf_t *qaconf);
static void free_(qaconf_t *qaconf);

static int _parse_inline(qaconf_t *qaconf, FILE *fp, uint8_t flags,
                         enum qaconf_section sectionid,
                         qaconf_cbdata_t *cbdata_parent);
static void _seterrmsg(qaconf_t *qaconf, const char *format, ...);
static void _free_cbdata(qaconf_cbdata_t *cbdata);
static int _is_number_str(const char *s);
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
    qaconf->reseterror     = reseterror;
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

    // Set info
    if (qaconf->filepath != NULL) free(qaconf->filepath);
    qaconf->filepath = strdup(filepath);
    qaconf->lineno = 0;

    // Parse
    int optcount = _parse_inline(qaconf, fp, flags, QAC_SECTION_ROOT, NULL);

    // Clean up
    fclose(fp);

    return optcount;
}

static const char *errmsg(qaconf_t *qaconf)
{
    return (const char*)qaconf->errstr;
}

static void reseterror(qaconf_t *qaconf)
{
    if (qaconf->errstr != NULL) {
        free(qaconf->errstr);
        qaconf->errstr = NULL;
    }
}

static void free_(qaconf_t *qaconf)
{
    if (qaconf->filepath != NULL) free(qaconf->filepath);
    if (qaconf->errstr != NULL) free(qaconf->errstr);
    if (qaconf->options != NULL) free(qaconf->options);
    free(qaconf);
}

#ifndef _DOXYGEN_SKIP
#define ARGV_INIT_SIZE  (4)
#define ARGV_INCR_STEP  (8)
#define MAX_TYPECHECK   (6)
static int _parse_inline(qaconf_t *qaconf, FILE *fp, uint8_t flags,
                         enum qaconf_section sectionid,
                         qaconf_cbdata_t *cbdata_parent)
{
    // Assign compare function.
    int (*cmpfunc)(const char *, const char *) = strcmp;
    if (flags & QAC_CASEINSENSITIVE) cmpfunc = strcasecmp;

    char buf[MAX_LINESIZE];
    bool doneloop = false;
    bool exception = false;
    int optcount = 0;  // number of option entry processed.
    int newsectionid = 0;  // temporary store
    while (doneloop == false && exception == false) {

#define EXITLOOP(fmt, args...) do {                                         \
    _seterrmsg(qaconf, "%s:%d " fmt,                                        \
               qaconf->filepath, qaconf->lineno, ##args);                   \
    exception = true;                                                       \
    goto exitloop;                                                          \
} while (0);

        if (fgets(buf, MAX_LINESIZE, fp) == NULL) {
            // Check if section was opened and never closed
            if (cbdata_parent != NULL) {
                EXITLOOP("<%s> section was not closed.",
                          cbdata_parent->argv[0]);
            }
            break;
        }

        // Increase line number counter
        qaconf->lineno++;

        // Trim white spaces
        qstrtrim(buf);

        // Skip blank like and comments.
        if (IS_EMPTY_STR(buf) || *buf == '#') {
            continue;
        }

        DEBUG("%s (line=%d)", buf, qaconf->lineno);

        // Create a callback data
        qaconf_cbdata_t *cbdata = (qaconf_cbdata_t*)malloc(sizeof(qaconf_cbdata_t));
        ASSERT(cbdata != NULL);
        memset(cbdata, '\0', sizeof(qaconf_cbdata_t));
        if (cbdata_parent != NULL) {
            cbdata->section = sectionid ;
            cbdata->sections = cbdata_parent->sections | sectionid;
            cbdata->level = cbdata_parent->level + 1;
            cbdata->parent = cbdata_parent;
        } else {
            cbdata->section = sectionid;
            cbdata->sections = sectionid;
            cbdata->level = 0;
            cbdata->parent = NULL;
        }

        // Escape section option
        char *sp = buf;
        if (*sp == '<') {
            if (ENDING_CHAR(sp) != '>') {
                EXITLOOP("Missing closing bracket. - '%s'.", buf);
            }

            sp++;
            if (*sp == '/') {
                cbdata->otype = QAC_OTYPE_SECTIONCLOSE;
                sp++;
            } else {
                cbdata->otype = QAC_OTYPE_SECTIONOPEN;
            }

            // Remove tailing bracket
            ENDING_CHAR(sp) = '\0';
        } else {
            cbdata->otype = QAC_OTYPE_OPTION;
        }

        // Brackets has removed at this point
        // Copy data into cbdata buffer.
        cbdata->data = strdup(sp);
        ASSERT(cbdata->data != NULL);

        // Parse and tokenize.
        int argvsize = 0;
        char *wp1, *wp2;
        bool doneparsing = false;
        for (wp1 = (char *)cbdata->data; doneparsing == false; wp1 = wp2) {
            // Allocate/Realloc argv array
            if (argvsize == cbdata->argc) {
                argvsize += (argvsize == 0) ? ARGV_INIT_SIZE : ARGV_INCR_STEP;
                cbdata->argv = (char**)realloc((void *)cbdata->argv, sizeof(char*) * argvsize);
                ASSERT(cbdata->argv != NULL);
            }

	    // Skip whitespaces
            for (; (*wp1 == ' ' || *wp1 == '\t'); wp1++);

            // Quote handling
            int qtmark = 0;  // 1 for singlequotation, 2 for doublequotation
            if (*wp1 == '\'') {
                qtmark = 1;
                wp1++;
            } else if (*wp1 == '"') {
                qtmark = 2;
                wp1++;
            }

            // Parse a word
            for (wp2 = wp1; ; wp2++) {
                if (*wp2 == '\0') {
                    doneparsing = true;
                    break;
                } else if (*wp2 == '\'') {
                    if (qtmark == 1) {
                        qtmark = 0;
                        break;
                    }
                } else if (*wp2 == '"') {
                    if (qtmark == 2) {
                        qtmark = 0;
                        break;
                    }
                } else if (*wp2 == '\\') {
                    if (qtmark > 0) {
                        size_t wordlen = wp2 - wp1;
                        if (wordlen > 0) memmove(wp1 + 1, wp1, wordlen);
                        wp1++;
                        wp2++;
                    }
                } else if (*wp2 == ' ' || *wp2 == '\t') {
                    if (qtmark == 0) break;
                }
            }
            *wp2 = '\0';
            wp2++;

            // Check quotations has paired.
            if (qtmark > 0) {
                EXITLOOP("Quotation hasn't properly closed.");
            }

            // Store a argument
            cbdata->argv[cbdata->argc] = wp1;
            cbdata->argc++;
            DEBUG("  argv[%d]=%s", cbdata->argc - 1, wp1);
        }

        // Check mismatch sectionclose
        if (cbdata->otype == QAC_OTYPE_SECTIONCLOSE) {
            if (cbdata_parent == NULL ||
                cmpfunc(cbdata->argv[0], cbdata_parent->argv[0])) {
                EXITLOOP("Trying to close <%s> section that wasn't opened.",
                          cbdata->argv[0]);
            }
        }

        // Find matching option
        bool optfound = false;
        int i;
        for (i = 0; i < qaconf->numoptions; i++) {
            qaconf_option_t *option = &qaconf->options[i];

            if (!cmpfunc(cbdata->argv[0], option->name)) {
                optfound = true;

                // Check sections
                if ((cbdata->otype != QAC_OTYPE_SECTIONCLOSE) &&
                    (option->sections != QAC_SECTION_ALL) &&
                    (option->sections & sectionid) == 0) {
                    EXITLOOP("Option '%s' is in wrong section.", option->name);
                }

                // Check argument types
                if (cbdata->otype != QAC_OTYPE_SECTIONCLOSE) {
                    // Check number of arguments
                    int numtake = option->take & QAC_TAKEALL;
                    if (numtake != QAC_TAKEALL && numtake != (cbdata->argc -1 )) {
                        EXITLOOP("'%s' option takes only %d arguments.",
                                 option->name, numtake);
                    }

                    // Check argument types
                    int j;
                    for (j = 1; j < cbdata->argc && j <= MAX_TYPECHECK; j++) {
                        if (option->take & (QAC_A1_FLOAT << (j - 1))) {
                            // floating point type
                            if(_is_number_str(cbdata->argv[j]) == 0) {
                                EXITLOOP("%dth argument of '%s' must be a floating point.",
                                         j, option->name);
                            }
                        } else if (option->take & (QAC_A1_INT << (j - 1))) {
                            // integer type
                            if(_is_number_str(cbdata->argv[j]) != 1) {
                                EXITLOOP("%dth argument of '%s' must be a integer.",
                                         j, option->name);
                            }
                        }
                    }
                }

                // Callback
                //DEBUG("Callback %s", option->name);
                qaconf_cb_t *usercb = option->cb;
                if (usercb == NULL) usercb = qaconf->defcb;
                if (usercb != NULL) {
                    char *cberrmsg = NULL;

                    if (cbdata->otype != QAC_OTYPE_SECTIONCLOSE) {
                        // Normal option and sectionopen
                        cberrmsg = usercb(cbdata, qaconf->userdata);
                    } else {
                        // QAC_OTYPE_SECTIONCLOSE

                        // Change otype
                        ASSERT(cbdata_parent != NULL);
                        enum qaconf_otype orig_otype = cbdata_parent->otype;
                        cbdata_parent->otype = QAC_OTYPE_SECTIONCLOSE;

                        // Callback
                        cberrmsg = usercb(cbdata_parent, qaconf->userdata);

                        // Restore otype
                        cbdata_parent->otype = orig_otype;

                    }

                    // Error handling
                    if (cberrmsg != NULL) {
                        EXITLOOP("%s", cberrmsg);
                    }
                }

                // Duplicated option handlers are allowed.
                if (cbdata->otype != QAC_OTYPE_OPTION) {
                    if (cbdata->otype == QAC_OTYPE_SECTIONOPEN) {
                        // Store it for later
                        newsectionid = option->sectionid;
                    }
                    break;
                }
            }
        }

        // If not found.
        if (optfound == false) {
            if (qaconf->defcb != NULL) {
                qaconf->defcb(cbdata, qaconf->userdata);
            } else if ((flags & QAC_IGNOREUNKNOWN) == 0) {
                EXITLOOP("Unregistered option '%s'.", cbdata->argv[0]);
            }
        }

        // Section handling
        if (cbdata->otype == QAC_OTYPE_SECTIONOPEN) {
            // Enter recursive call
            DEBUG("Entering next level %d.", cbdata->level+1);
            int optcount2 = _parse_inline(qaconf, fp, flags,
                                          newsectionid, cbdata);
            if (optcount2 >= 0) {
                optcount += optcount2;
            } else {
                exception = true;
            }
            DEBUG("Returned to previous level %d.", cbdata->level);
        } else if (cbdata->otype == QAC_OTYPE_SECTIONCLOSE) {
            // Leave recursive call
            // TODO: check bracket
            doneloop = true;
        }

        exitloop:
        // Release resources
        if (cbdata != NULL) {
            _free_cbdata(cbdata);
            cbdata = NULL;
        }

        if (exception == true) {
            break;
        }

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

static void _free_cbdata(qaconf_cbdata_t *cbdata)
{
    if (cbdata->argv != NULL) free(cbdata->argv);
    if (cbdata->data != NULL) free(cbdata->data);
    free(cbdata);
}

// return 2 for floating point .
// return 1 for integer
// return 0 for non number
static int _is_number_str(const char *s)
{
    char *op = (char *)s;
    if (*op == '-') {
        op++;
    }

    char *cp, *dp;
    for (cp = op, dp = NULL; *cp != '\0'; cp++) {
        if ('0' <= *cp  && *cp <= '9') {
            continue;
        }

        if (*cp == '.') {
            if (cp == op) return 0;  // dot can't be at the beginning.
            if (dp != NULL) return 0;  // dot can't be appeared more than once.
            dp = cp;
            continue;
        }

        return 0;
    }

    if (cp == op) {
        return 0;  // empty string
    }

    if (dp != NULL) {
        if (dp + 1 == cp) return 0;  // dot can't be at the end.
        return 2;  // float point
    }

    // integer
    return 1;
}

#endif /* _DOXYGEN_SKIP */

#endif /* DISABLE_QACONF */

