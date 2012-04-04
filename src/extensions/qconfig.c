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
 * @file qconfig.c Configuration parser APIs.
 */

#ifndef DISABLE_QCONFIG

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "qlibc.h"
#include "qlibcext.h"
#include "qinternal.h"

#define _INCLUDE_DIRECTIVE  "@INCLUDE "

/**
 * Load & parse configuration file
 *
 * @param tbl       a pointer of qlisttbl_t. NULL can be used for empty.
 * @param filepath  configuration file path
 * @param sepchar   separater used in configuration file to divice key and value
 *
 * @return a pointer of qlisttbl_t in case of successful,
 *  otherwise(file not found) returns NULL
 *
 * @code
 *   # This is "config.conf" file.
 *   # A line which starts with # character is comment
 *
 *   @INCLUDE config.def  => include 'config.def' file.
 *
 *   prefix=/tmp        => set static value. 'prefix' is the key for this entry.
 *   log=${prefix}/log  => get the value from previously defined key 'prefix'.
 *   user=${%USER}      => get environment variable.
 *   host=${!/bin/hostname -s}  => run external command and put it's output.
 *   id=${user}@${host}
 *
 *   # now entering into 'system' section.
 *   [system]           => a key 'system.' with value 'system' will be inserted.
 *   ostype=${%OSTYPE}  => 'system.ostype' is the key for this entry.
 *   machtype=${%MACHTYPE}  => 'system.machtype' is the key for this entry.
 *
 *   # entering into 'daemon' section.
 *   [daemon]
 *   port=1234
 *   name=${user}_${host}_${system.ostype}_${system.machtype}
 *
 *   # escape section. (go back to root)
 *   []
 *   rev=822
 * @endcode
 *
 * @code
 *   # This is "config.def" file.
 *   prefix = /usr/local
 *   bin = ${prefix}/bin
 *   log = ${prefix}/log
 *   user = unknown
 *   host = unknown
 * @endcode
 *
 * @code
 *   qlisttbl_t *tbl = qconfig_parse_file(NULL, "config.conf", '=');
 *   tbl->debug(tbl, stdout);
 *
 *   [Output]
 *   bin=/usr/local/bin? (15)
 *   prefix=/tmp? (5)
 *   log=/tmp/log? (9)
 *   user=seungyoung.kim? (9)
 *   host=eng22? (6)
 *   id=seungyoung.kim@eng22? (15)
 *   system.=system? (7)
 *   system.ostype=linux? (6)
 *   system.machtype=x86_64? (7)
 *   daemon.=daemon? (7)
 *   daemon.port=1234? (5)
 *   daemon.name=seungyoung.kim_eng22_linux_x86_64? (28)
 *   rev=822? (4)
 * @endcode
 */
qlisttbl_t *qconfig_parse_file(qlisttbl_t *tbl, const char *filepath,
                               char sepchar)
{
    char *str = qfile_load(filepath, NULL);
    if (str == NULL) return NULL;

    // process include directive
    char *strp = str;;
    while ((strp = strstr(strp, _INCLUDE_DIRECTIVE)) != NULL) {
        if (strp == str || strp[-1] == '\n') {
            char buf[PATH_MAX];

            // parse filename
            char *tmpp;
            for (tmpp = strp + CONST_STRLEN(_INCLUDE_DIRECTIVE); *tmpp != '\n'
                 && *tmpp != '\0'; tmpp++);
            int len = tmpp - (strp + CONST_STRLEN(_INCLUDE_DIRECTIVE));
            if (len >= sizeof(buf)) {
                DEBUG("Can't process %s directive.", _INCLUDE_DIRECTIVE);
                free(str);
                return NULL;
            }

            strncpy(buf, strp + CONST_STRLEN(_INCLUDE_DIRECTIVE), len);
            buf[len] = '\0';
            qstrtrim(buf);

            // get full file path
            if (!(buf[0] == '/' || buf[0] == '\\')) {
                char tmp[PATH_MAX];
                char *dir = qfile_get_dir(filepath);
                if (strlen(dir) + 1 + strlen(buf) >= sizeof(buf)) {
                    DEBUG("Can't process %s directive.", _INCLUDE_DIRECTIVE);
                    free(dir);
                    free(str);
                    return NULL;
                }
                snprintf(tmp, sizeof(tmp), "%s/%s", dir, buf);
                free(dir);

                strcpy(buf, tmp);
            }

            // read file
            char *incdata;
            if (strlen(buf) == 0 || (incdata = qfile_load(buf, NULL)) == NULL) {
                DEBUG("Can't process '%s%s' directive.", _INCLUDE_DIRECTIVE,
                      buf);
                free(str);
                return NULL;
            }

            // replace
            strncpy(buf, strp, CONST_STRLEN(_INCLUDE_DIRECTIVE) + len);
            buf[CONST_STRLEN(_INCLUDE_DIRECTIVE) + len] = '\0';
            strp = qstrreplace("sn", str, buf, incdata);
            free(incdata);
            free(str);
            str = strp;
        } else {
            strp += CONST_STRLEN(_INCLUDE_DIRECTIVE);
        }
    }

    // parse
    tbl = qconfig_parse_str(tbl, str, sepchar);
    free(str);

    return tbl;
}

/**
 * Parse string
 *
 * @param tbl       a pointer of qlisttbl_t. NULL can be used for empty.
 * @param str       key, value pair strings
 * @param sepchar   separater used in configuration file to divice key and value
 *
 * @return a pointer of qlisttbl_t in case of successful,
 *         otherwise(file not found) returns NULL
 *
 * @see qconfig_parse_file
 *
 * @code
 *  qlisttbl_t *tbl = qconfig_parse_str(NULL, "key = value\nhello = world", '=');
 * @endcode
 */
qlisttbl_t *qconfig_parse_str(qlisttbl_t *tbl, const char *str, char sepchar)
{
    if (str == NULL) return NULL;

    if (tbl == NULL) {
        tbl = qlisttbl();
        if (tbl == NULL) return NULL;
    }

    char *section = NULL;
    char *org, *buf, *offset;
    for (org = buf = offset = strdup(str); *offset != '\0'; ) {
        // get one line into buf
        for (buf = offset; *offset != '\n' && *offset != '\0'; offset++);
        if (*offset != '\0') {
            *offset = '\0';
            offset++;
        }
        qstrtrim(buf);

        // skip blank or comment line
        if ((buf[0] == '#') || (buf[0] == '\0')) continue;

        // section header
        if ((buf[0] == '[') && (buf[strlen(buf) - 1] == ']')) {
            // extract section name
            if (section != NULL) free(section);
            section = strdup(buf + 1);
            section[strlen(section) - 1] = '\0';
            qstrtrim(section);

            // remove section if section name is empty. ex) []
            if (section[0] == '\0') {
                free(section);
                section = NULL;
                continue;
            }

            // in order to put 'section.=section'
            sprintf(buf, "%c%s", sepchar, section);
        }

        // parse & store
        char *value = strdup(buf);
        char *name  = _q_makeword(value, sepchar);
        qstrtrim(value);
        qstrtrim(name);

        // put section name as a prefix
        if (section != NULL) {
            char *newname = qstrdupf("%s.%s", section, name);
            free(name);
            name = newname;
        }

        // get parsed string
        char *newvalue = tbl->parse_str(tbl, value);
        if (newvalue != NULL) {
            tbl->put_str(tbl, name, newvalue, true);
            free(newvalue);
        }

        free(name);
        free(value);
    }
    free(org);
    if (section != NULL) free(section);

    return tbl;
}

#endif /* DISABLE_QCONFIG */
