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
 *
 * Apache-style Configuration is a configuration file syntax and format
 * originally introduced by Apache HTTPd project. This format is power,
 * versatile, flexible and human friendly.
 *
 * Sample Apache-style Configuration Syntax:
 * @code
 *   # Lines that begin with the hash character "#" are considered comments.
 *   Listen 53
 *   Protocols UDP TCP
 *   IPSEC On
 *
 *   <Domain "qdecoder.org">
 *     TTL 86400
 *     MX 10 mail.qdecoder.org
 *
 *     <Host mail>
 *        IPv4 192.168.10.1
 *        TXT "US Rack-13D-18 \"San Jose's\""
 *     </Host>
 *
 *     <Host www>
 *        IPv4 192.168.10.2
 *        TXT 'KR Rack-48H-31 "Seoul\'s"'
 *        TTL 3600
 *     </Host>
 *   </Domain>
 *
 *   <Domain "ringfs.org">
 *     <Host www>
 *        CNAME www.qdecoder.org
 *     </Host>
 *   </Domain>
 * @endcode
 *
 * @code
 *   // Define scope.
 *   //   QAC_SCOPE_ALL and QAC_SCOPE_ROOT are predefined.
 *   //   Custum scope should be defined from 2(1 << 1).
 *   //   Note) These values are ORed(bit operation), so the number should be
 *   //         2(1<<1), 4(1<<2), 6(1<<3), 8(1<<4), ...
 *   enum {
 *     OPT_SECTION_ALL        = QAC_SECTION_ALL,   // pre-defined
 *     OPT_SECTION_ROOT       = QAC_SECTION_ROOT,  // pre-defined
 *     OPT_SECTION_DOMAIN     = (1 << 1),    // user-defined section
 *     OPT_SECTION_HOST       = (1 << 2),    // user-defined section
 *   };
 *
 *   // Define callback proto-types.
 *   static QAC_CB(confcb_debug);
 *   static QAC_CB(confcb_userdata_example);
 *   static QAC_CB(confcb_section_example);
 *
 *   // Define options and callbacks.
 *   static qaconf_option_t options[] = {
 *     {"Listen", QAC_TAKE_INT, confcb_userdata_example, 0, OPT_SECTION_ALL},
 *     {"Protocols", QAC_TAKEALL, confcb_debug, 0, OPT_SECTION_ROOT},
 *     {"IPSEC", QAC_TAKE_BOOL, confcb_debug, 0, OPT_SECTION_ROOT},
 *     {"Domain", QAC_TAKE_STR, confcb_debug, OPT_SECTION_DOMAIN, OPT_SECTION_ROOT},
 *     {  "TTL", QAC_TAKE_INT, confcb_debug, 0, OPT_SECTION_DOMAIN | OPT_SECTION_HOST},
 *     {  "MX", QAC_TAKE2 | QAC_A1_INT, confcb_debug, 0, OPT_SECTION_DOMAIN},
 *     {  "Host", QAC_TAKE_STR, confcb_section_example, OPT_SECTION_HOST, OPT_SECTION_DOMAIN},
 *     {    "IPv4", QAC_TAKE_STR, confcb_debug, 0, OPT_SECTION_HOST},
 *     {    "TXT", QAC_TAKE_STR, confcb_debug, 0, OPT_SECTION_HOST},
 *     {    "CNAME", QAC_TAKE_STR, confcb_debug, 0, OPT_SECTION_HOST},
 *     QAC_OPTION_END
 *   };
 *
 *   int user_main(void)
 *   {
 *     // Create a userdata structure.
 *     struct MyConf myconf;
 *
 *     // Initialize and create a qaconf object.
 *     qaconf_t *conf = qaconf();
 *     if (conf == NULL) {
 *       printf("Failed to open '" CONF_PATH "'.\n");
 *       return -1;
 *     }
 *
 *     // Register options.
 *     conf->addoptions(conf, options);
 *
 *     // Set callback userdata.
 *     conf->setuserdata(conf, &myconf);
 *
 *     // Run parser.
 *     int count = conf->parse(conf, CONF_PATH, QAC_CASEINSENSITIVE);
 *     if (count < 0) {
 *       printf("Error: %s\n", conf->errmsg(conf));
 *     } else {
 *       printf("Successfully loaded.\n");
 *     }
 *
 *     // Verify userdata structure.
 *     if (conf->errmsg(conf) == NULL) {  // another way to check parsing error.
 *       // codes here
 *     }
 *
 *     // Release resources.
 *     conf->free(conf);
 *   }
 *
 *   static QAC_CB(confcb_debug)
 *   {
 *     int i;
 *     for (i = 0; i < data->level; i++) {
 *       printf ("    ");
 *     }
 *
 *     // Print option name
 *     if (data->otype == QAC_OTYPE_SECTIONOPEN) {
 *       printf("<%s>", data->argv[0]);
 *     } else if (data->otype == QAC_OTYPE_SECTIONCLOSE) {
 *       printf("</%s>", data->argv[0]);
 *     } else {  // This is QAC_OTYPE_OPTION type.
 *       printf("%s", data->argv[0]);
 *     }
 *
 *     // Print parent names
 *     qaconf_cbdata_t *parent;
 *     for (parent = data->parent; parent != NULL; parent = parent->parent) {
 *       printf(" ::%s(%s)", parent->argv[0], parent->argv[1]);
 *     }
 *
 *     // Print option arguments
 *     for (i = 1; i < data->argc; i++) {
 *       printf(" [%d:%s]", i, data->argv[i]);
 *     }
 *     printf("\n");
 *
 *     // Return OK
 *     return NULL;
 *   }
 * @endcode
 *
 * @code
 * @endcode
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
static int _is_str_number(const char *s);
static int _is_str_bool(const char *s);
#endif

/**
 * Create a new configuration object.
 *
 * @return a pointer of new qaconf_t object.
 *
 * @code
 *   qaconf_t *conf = qaconf();
 *   if (conf == NULL) {
 *     // Insufficient memory.
 *   } 
 * @endcode
 */
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

/**
 * Register option directives.
 *
 * @param qaconf qaconf_t object
 * @param options array pointer of qaconf_option_t.
 *
 * @return a number of options registered(added).
 */
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

/**
 * Set default callback function.
 *
 * Default callback function will be called for unregistered option directives.
 * QAC_IGNOREUNKNOWN flag will be ignored when default callback has set.
 *
 * @param qaconf qaconf_t object
 * @param callback callback function pointer
 */
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
#define MAX_TYPECHECK   (5)
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
    void *freethis = NULL;  // userdata to free
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

            // For quoted string, this case can be happened.
            if (*wp2 == '\0') {
                doneparsing = true;
            }
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
        for (i = 0; optfound == false && i < qaconf->numoptions; i++) {
            qaconf_option_t *option = &qaconf->options[i];

            if (!cmpfunc(cbdata->argv[0], option->name)) {
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
                        EXITLOOP("'%s' option takes %d arguments.",
                                 option->name, numtake);
                    }

                    // Check argument types
                    int deftype; // 0:str, 1:int, 2:float, 3:bool
                    if (option->take & QAC_AA_INT) deftype = 1;
                    else if (option->take & QAC_AA_FLOAT) deftype = 2;
                    else if (option->take & QAC_AA_BOOL) deftype = 3;
                    else deftype = 0;

                    int j;
                    for (j = 1; j < cbdata->argc && j <= MAX_TYPECHECK; j++) {
                        int argtype;
                        if (option->take & (QAC_A1_INT << (j - 1))) argtype = 1;
                        else if (option->take & (QAC_A1_FLOAT << (j - 1))) argtype = 2;
                        else if (option->take & (QAC_A1_BOOL << (j - 1))) argtype = 3;
                        else argtype = deftype;

                        if (argtype == 1) {
                            // integer type
                            if(_is_str_number(cbdata->argv[j]) != 1) {
                                EXITLOOP("%dth argument of '%s' must be integer type.",
                                         j, option->name);
                            }
                        } else if (argtype == 2) {
                            // floating point type
                            if(_is_str_number(cbdata->argv[j]) == 0) {
                                EXITLOOP("%dth argument of '%s' must be floating point. type",
                                         j, option->name);
                            }
                        } else if (argtype == 3) {
                            // bool type
                            if(_is_str_bool(cbdata->argv[j]) != 0) {
                                // Change argument to "1".
                                strcpy(cbdata->argv[j], "1");
                            } else {
                                EXITLOOP("%dth argument of '%s' must be bool type.",
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

                        // Restore type
                        cbdata_parent->otype = orig_otype;
                    }

                    // Error handling
                    if (cberrmsg != NULL) {
                        freethis = cberrmsg;
                        EXITLOOP("%s", cberrmsg);
                    }
                }

                if (cbdata->otype == QAC_OTYPE_SECTIONOPEN) {
                    // Store it for later
                    newsectionid = option->sectionid;
                }

                // Set found flag
                optfound = true;
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
            doneloop = true;
        }

        exitloop:
        // Release resources
        if (freethis != NULL) {
            free(freethis);
        }

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
static int _is_str_number(const char *s)
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

static int _is_str_bool(const char *s)
{
    if (!strcasecmp(s, "true")) return 1;
    else if (!strcasecmp(s, "on")) return 1;
    else if (!strcasecmp(s, "yes")) return 1;
    else if (!strcasecmp(s, "1")) return 1;
    return 0;
}

#endif /* _DOXYGEN_SKIP */

#endif /* DISABLE_QACONF */

