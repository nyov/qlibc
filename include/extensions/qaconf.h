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
 * $Id: qlibc.h 55 2012-04-03 03:15:00Z seungyoung.kim $
 ******************************************************************************/

/**
 * Apache-style Configuration File Parser.
 *
 * This is a qLibc extension implementing Apache-style configuration file
 * parser.
 *
 * @file qaconf.h
 */

#ifndef _QACONF_H
#define _QACONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "qlibc.h"

/* types */
typedef struct qaconf_s qaconf_t;
typedef struct qaconf_option_s qaconf_option_t;
typedef struct qaconf_cbdata_s qaconf_cbdata_t;
typedef char *(qaconf_cb_t) (qaconf_cbdata_t *data, void *userdata);

/* public functions */
extern qaconf_t *qaconf(void);

/* user's callback function prototype */
#define QAC_CB(func) char *func(qaconf_cbdata_t *data, void *userdata)

/* parser option */
enum {
    QAC_CASEINSENSITIVE     = (1),
    QAC_IGNOREUNKNOWN       = (2),  /* Ignore unknown option */
};

/**
 * option type check (up to 6 arguments)
 *
 *   Double   Integer  Args
 *   ----------------------
 *   842184 | 218421 | 8421    (16 bit : 6bit + 6 bit + 4 bit)
 *   ----------------------
 *   000000   000000   0001  => QAC_TAKE1
 *   000000   000001   0001  => QAC_TAKE1_INT
 *   000000   000010   0010  => QAC_TAKE2_STR_INT
 *   000100   000010   0011  => QAC_TAKE3 | QAC_A2_INT | QAC_A3_FLOAT
 *   000000   000000   1111  => QAC_TAKE_ALL
 *   100000   010100   1111  => QAC_TAKE_ALL|QAC_A3_INT|QAC_A5_INT|QAC_A6_FLOAT
 */
enum qaconf_take {
    QAC_TAKE0           = 0,
    QAC_TAKE1           = 1,
    QAC_TAKE2           = 2,
    QAC_TAKE3           = 3,
    QAC_TAKE4           = 4,
    QAC_TAKE5           = 5,
    QAC_TAKE6           = 6,
    QAC_TAKEALL         = 0xF, /* Take any number of elements. (0 ~ INT_MAX) */

    QAC_A1_STR          = 0,
    QAC_A2_STR          = 0,
    QAC_A3_STR          = 0,
    QAC_A4_STR          = 0,
    QAC_A5_STR          = 0,
    QAC_A6_STR          = 0,

    QAC_A1_INT          = (1 << 4),
    QAC_A2_INT          = (QAC_A1_INT << 1),
    QAC_A3_INT          = (QAC_A1_INT << 2),
    QAC_A4_INT          = (QAC_A1_INT << 3),
    QAC_A5_INT          = (QAC_A1_INT << 4),
    QAC_A6_INT          = (QAC_A1_INT << 5),

    QAC_A1_FLOAT        = (1 << 10),
    QAC_A2_FLOAT        = (QAC_A1_FLOAT << 1),
    QAC_A3_FLOAT        = (QAC_A1_FLOAT << 2),
    QAC_A4_FLOAT        = (QAC_A1_FLOAT << 3),
    QAC_A5_FLOAT        = (QAC_A1_FLOAT << 4),
    QAC_A6_FLOAT        = (QAC_A1_FLOAT << 5),

    QAC_TAKE1_STR       = (QAC_TAKE1 | QAC_A1_STR),
    QAC_TAKE1_INT       = (QAC_TAKE1 | QAC_A1_INT),
    QAC_TAKE1_FLOAT     = (QAC_TAKE1 | QAC_A1_FLOAT),
};

/* pre-defined sections */
enum qaconf_section {
    QAC_SECTION_ALL  = (0),        /* pre-defined */
    QAC_SECTION_ROOT = (1)         /* pre-defined */
};

/* option type */
enum qaconf_otype {
    QAC_OTYPE_OPTION     = 0,    /* general option */
    QAC_OTYPE_SECTIONOPEN  = 1,  /* section open <XXX> */
    QAC_OTYPE_SECTIONCLOSE = 2   /* section close </XXX> */
};

/**
 * qaconf_t object.
 */
struct qaconf_s {
    /* capsulated member functions */
    int (*addoptions) (qaconf_t *qaconf, const qaconf_option_t *options);
    void (*setdefhandler) (qaconf_t *qaconf, const qaconf_cb_t *callback);
    void (*setuserdata) (qaconf_t *qaconf, const void *userdata);
    int (*parse) (qaconf_t *qaconf, const char *filepath, uint8_t flags);
    const char *(*errmsg) (qaconf_t *qaconf);
    void (*reseterror) (qaconf_t *qaconf);
    void (*free) (qaconf_t *qaconf);

    /* private variables - do not access directly */
    int numoptions;             /*!< a number of user defined options */
    qaconf_option_t *options;   /*!< option data */

    qaconf_cb_t *defcb;         /*!< default callback for unregistered option */
    void *userdata;             /*!< userdata */

    char *filepath;             /*!< current processing file's path */
    int lineno;                 /*!< current processing line number */

    char *errstr;               /*!< last error string */
};

/**
 * structure for user option definition.
 */
struct qaconf_option_s {
    char *name;             /*!< name of option. */
    uint32_t take;          /*!< number of arguments and type checking */
    qaconf_cb_t *cb;        /*!< callback function */
    uint64_t sectionid;     /*!< sectionid if this is a section */
    uint64_t sections;      /*!< ORed sectionid(s) where this option is allowed */
};
#define QAC_OPTION_END  {NULL, 0, NULL, 0, 0}

/**
 * callback data structure.
 */
struct qaconf_cbdata_s {
    enum qaconf_otype otype;  /*!< option type */
    uint64_t section;         /*!< current section where this option is placed */
    uint64_t sections;        /*!< ORed all parent's sectionid(s) including current sections */
    uint8_t level;            /*!< number of parents, root level is 0 */
    qaconf_cbdata_t *parent;  /*!< upper parent link */

    int argc;       /*!< number arguments. always equal or greater than 1. */
    char **argv;    /*!< argument pointers. argv[0] is option name. */

    void *data;     /*!< where actual data is stored */
};

#ifdef __cplusplus
}
#endif

#endif /*_QACONF_H */
