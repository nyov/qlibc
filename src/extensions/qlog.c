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
 * @file qlog.c Rotating file logger object.
 *
 * qlog_t implements a auto-rotating file logger.
 *
 * @code
 *   // create a daily-rotating log file.
 *   qlog_t *log = qlog("/tmp/dailylog-%Y%m%d.err", 0644, 86400, false);
 *   if(log == NULL) return;
 *
 *   // screen out.
 *   log->duplicate(log, stdout, true);
 *
 *   // write logs.
 *   log->write(log, "Service started.");
 *   log->writef(log, "Server Id: %d", 1);
 *
 *   // close and release resources.
 *   log->free(log);
 * @endcode
 */

#ifndef DISABLE_QLOG

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "qlibc.h"
#include "qlibcext.h"
#include "qinternal.h"

#ifndef _DOXYGEN_SKIP
static bool write_(qlog_t *log, const char *str);
static bool writef(qlog_t *log, const char *format, ...);
static bool duplicate(qlog_t *log, FILE *outfp, bool flush);
static bool flush_(qlog_t *log);
static bool free_(qlog_t *log);

// internal usages
static bool _real_open(qlog_t *log);
#endif

/**
 * Open ratating-log file
 *
 * @param filepathfmt     filename format. formatting argument is same as
 *                        strftime()
 * @param mode            new file mode. 0 for system default
 * @param rotateinterval  rotating interval seconds, set 0 to disable rotation
 * @param flush           set to true if you want to flush everytime logging.
 *                        false for buffered logging
 *
 * @return a pointer of qlog_t structure
 *
 * @note
 *  rotateinterval is not relative time. If you set it to 3600, log file will be
 *  rotated at every hour. And filenameformat is same as strftime(). So If you
 *  want to log with hourly rotating, filenameformat must be defined including
 *  hour format like "/somepath/xxx-%Y%m%d%H.log". You can set it to
 *  "/somepath/xxx-%H.log" for daily overrided log file.
 *
 * @code
 *   qlog_t *log = qlog("/tmp/qdecoder-%Y%m%d.err", 0644, 86400, false);
 *   log->free(log);
 * @endcode
 *
 * @note
 *  Use "--enable-threadsafe" configure script option to use under
 *  multi-threaded environments.
 */
qlog_t *qlog(const char *filepathfmt, mode_t mode, int rotateinterval,
             bool flush)
{
    qlog_t *log;

    /* malloc qlog_t structure */
    if ((log = (qlog_t *)malloc(sizeof(qlog_t))) == NULL) return NULL;

    /* fill structure */
    memset((void *)(log), 0, sizeof(qlog_t));
    qstrcpy(log->filepathfmt, sizeof(log->filepathfmt), filepathfmt);
    log->mode = mode;
    if (rotateinterval > 0) log->rotateinterval = rotateinterval;
    log->logflush = flush;

    if (_real_open(log) == false) {
        free(log);
        return NULL;
    }

    // member methods
    log->write  = write_;
    log->writef = writef;
    log->duplicate  = duplicate;
    log->flush  = flush_;
    log->free   = free_;

    // initialize recursive lock
    Q_MUTEX_INIT(log->qmutex, true);

    return log;
}

/**
 * qlog_t->write(): Log messages
 *
 * @param log       a pointer of qlog_t
 * @param str       message string
 *
 * @return true if successful, otherewise returns false
 */
static bool write_(qlog_t *log, const char *str)
{
    if (log == NULL || log->fp == NULL) return false;

    Q_MUTEX_ENTER(log->qmutex);

    /* duplicate stream */
    if (log->outfp != NULL) {
        fprintf(log->outfp, "%s\n", str);
        if (log->outflush == true) fflush(log->outfp);
    }

    /* check log rotate is needed*/
    if (log->nextrotate > 0 && time(NULL) >= log->nextrotate) {
        _real_open(log);
    }

    /* log to file */
    bool ret = false;
    if (fprintf(log->fp, "%s\n", str) >= 0) {
        if (log->logflush == true) fflush(log->fp);
        ret = true;
    }

    Q_MUTEX_LEAVE(log->qmutex);

    return ret;
}

/**
 * qlog_t->writef(): Log messages
 *
 * @param log       a pointer of qlog_t
 * @param format    messages format
 *
 * @return true if successful, otherewise returns false
 */
static bool writef(qlog_t *log, const char *format, ...)
{
    if (log == NULL || log->fp == NULL) return false;

    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) return false;

    bool ret = write_(log, str);

    free(str);
    return ret;
}

/**
 * qlog_t->duplicate(): Duplicate log string into other stream
 *
 * @param log       a pointer of qlog_t
 * @param fp        logging messages will be printed out into this stream.
 *                  set NULL to disable.
 * @param flush     set to true if you want to flush everytime duplicating.
 *
 * @return true if successful, otherewise returns false
 *
 * @code
 *   log->duplicate(log, stdout, true); // enable console out with flushing
 *   log->duplicate(log, stderr, false);    // enable console out
 *   log->duplicate(log, NULL, false);  // disable console out (default)
 * @endcode
 */
static bool duplicate(qlog_t *log, FILE *outfp, bool flush)
{
    if (log == NULL) return false;

    Q_MUTEX_ENTER(log->qmutex);
    log->outfp = outfp;
    log->outflush = flush;
    Q_MUTEX_LEAVE(log->qmutex);

    return true;
}

/**
 * qlog_t->flush(): Flush buffered log
 *
 * @param log       a pointer of qlog_t
 *
 * @return true if successful, otherewise returns false
 */
static bool flush_(qlog_t *log)
{
    if (log == NULL) return false;

    // only flush if flush flag is disabled
    Q_MUTEX_ENTER(log->qmutex);
    if (log->fp != NULL && log->logflush == false) fflush(log->fp);
    if (log->outfp != NULL && log->outflush == false) fflush(log->outfp);
    Q_MUTEX_LEAVE(log->qmutex);

    return false;
}

/**
 * qlog_t->free(): Close ratating-log file & de-allocate resources
 *
 * @param log       a pointer of qlog_t
 *
 * @return true if successful, otherewise returns false
 */
static bool free_(qlog_t *log)
{
    if (log == NULL) return false;

    flush_(log);
    Q_MUTEX_ENTER(log->qmutex);
    if (log->fp != NULL) {
        fclose(log->fp);
        log->fp = NULL;
    }
    Q_MUTEX_LEAVE(log->qmutex);
    Q_MUTEX_DESTROY(log->qmutex);
    free(log);
    return true;
}

#ifndef _DOXYGEN_SKIP

static bool _real_open(qlog_t *log)
{
    const time_t nowtime = time(NULL);

    /* generate filename */
    char newfilepath[PATH_MAX];
    strftime(newfilepath, sizeof(newfilepath), log->filepathfmt,
             localtime(&nowtime));

    /* open or re-open log file */
    if (log->fp == NULL) {
        log->fp = fopen(newfilepath, "a");
        if (log->fp == NULL) {
            DEBUG("_real_open: Can't open log file '%s'.", newfilepath);
            return false;
        }

        if (log->mode != 0) fchmod(fileno(log->fp), log->mode);
        qstrcpy(log->filepath, sizeof(log->filepath), newfilepath);
    } else if (strcmp(log->filepath, newfilepath)) {
        /* have opened stream, only reopen if new filename is different with
           existing one */
        FILE *newfp = fopen(newfilepath, "a");
        if (newfp != NULL) {
            if (log->mode != 0) fchmod(fileno(newfp), log->mode);
            fclose(log->fp);
            log->fp = newfp;
            qstrcpy(log->filepath, sizeof(log->filepath), newfilepath);
        } else {
            DEBUG("_real_open: Can't open log file '%s' for rotating.",
                  newfilepath);
        }
    } else {
        DEBUG("_real_open: skip re-opening log file.");
    }

    /* set next rotate time */
    if (log->rotateinterval > 0) {
        time_t ct = time(NULL);
        time_t dt = ct - mktime(gmtime(&ct));
        log->nextrotate = (((ct + dt) / log->rotateinterval) + 1)
                          * log->rotateinterval - dt;
    } else {
        log->nextrotate = 0;
    }

    return true;
}

#endif

#endif /* DISABLE_QLOG */
