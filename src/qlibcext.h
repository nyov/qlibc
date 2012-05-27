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
 * qlibc extension header file
 *
 * @file qlibcext.h
 */

#ifndef _QLIBCEXT_H
#define _QLIBCEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "qlibc.h"

/******************************************************************************
 * INI-style Configuration File Parser.
 * qconfig.c
 ******************************************************************************/

/* public functions */
extern qlisttbl_t *qconfig_parse_file(qlisttbl_t *tbl, const char *filepath,
                                      char sepchar, bool uniquekey);
extern qlisttbl_t *qconfig_parse_str(qlisttbl_t *tbl, const char *str,
                                     char sepchar, bool uniquekey);

/******************************************************************************
 * Apache-style Configuration File Parser.
 * qaconf.c
 ******************************************************************************/
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
 * option type check up to 6 arguments
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

/******************************************************************************
 * Rotating file logger.
 * qlog.c
 ******************************************************************************/

/* types */
typedef struct qlog_s qlog_t;

/* public functions */
extern qlog_t *qlog(const char *filepathfmt, mode_t mode, int rotateinterval,
                    bool flush);

/**
 * qlog structure
 */
struct qlog_s {
    /* capsulated member functions */
    bool (*write) (qlog_t *log, const char *str);
    bool (*writef) (qlog_t *log, const char *format, ...);
    bool (*duplicate) (qlog_t *log, FILE *outfp, bool flush);
    bool (*flush) (qlog_t *log);
    void (*free) (qlog_t *log);

    /* private variables - do not access directly */
    qmutex_t  qmutex;  /*!< activated if compiled with --enable-threadsafe */

    char filepathfmt[PATH_MAX]; /*!< file file naming format like
                                     /somepath/daily-%Y%m%d.log */
    char filepath[PATH_MAX];    /*!< generated system path of log file */
    FILE *fp;           /*!< file pointer of logpath */
    mode_t mode;        /*!< file mode */
    int rotateinterval; /*!< log file will be rotate in this interval seconds */
    int nextrotate;  /*!< next rotate universal time, seconds */
    bool logflush;   /*!< flag for immediate flushing */

    FILE *outfp;    /*!< stream pointer for duplication */
    bool outflush;  /*!< flag for immediate flushing for duplicated stream */
};

/******************************************************************************
 * HTTP client implementation.
 * qhttpclient.c
 ******************************************************************************/

/* types */
typedef struct qhttpclient_s  qhttpclient_t;

/* public functions */
extern qhttpclient_t *qhttpclient(const char *hostname, int port);

/**
 * qhttpclient structure
 */

struct qhttpclient_s {
    /* capsulated member functions */
    bool (*setssl) (qhttpclient_t *client);
    void (*settimeout) (qhttpclient_t *client, int timeoutms);
    void (*setkeepalive) (qhttpclient_t *client, bool keepalive);
    void (*setuseragent) (qhttpclient_t *client, const char *useragent);

    bool (*open) (qhttpclient_t *client);

    bool (*head) (qhttpclient_t *client, const char *uri, int *rescode,
                  qlisttbl_t *reqheaders, qlisttbl_t *resheaders);
    bool (*get) (qhttpclient_t *client, const char *uri, int fd,
                 off_t *savesize, int *rescode,
                 qlisttbl_t *reqheaders, qlisttbl_t *resheaders,
                 bool (*callback) (void *userdata, off_t recvbytes),
                 void *userdata);
    bool (*put) (qhttpclient_t *client, const char *uri, int fd,
                 off_t length, int *retcode, qlisttbl_t *userheaders,
                 qlisttbl_t *resheaders,
                 bool (*callback) (void *userdata, off_t sentbytes),
                 void *userdata);
    void *(*cmd) (qhttpclient_t *client,
                  const char *method, const char *uri,
                  void *data, size_t size, int *rescode,
                  size_t *contentslength,
                  qlisttbl_t *reqheaders, qlisttbl_t *resheaders);

    bool (*sendrequest) (qhttpclient_t *client, const char *method,
                         const char *uri, qlisttbl_t *reqheaders);
    int (*readresponse) (qhttpclient_t *client, qlisttbl_t *resheaders,
                         off_t *contentlength);

    ssize_t (*gets) (qhttpclient_t *client, char *buf, size_t bufsize);
    ssize_t (*read) (qhttpclient_t *client, void *buf, size_t nbytes);
    ssize_t (*write) (qhttpclient_t *client, const void *buf,
                      size_t nbytes);
    off_t (*recvfile) (qhttpclient_t *client, int fd, off_t nbytes);
    off_t (*sendfile) (qhttpclient_t *client, int fd, off_t nbytes);

    bool (*close) (qhttpclient_t *client);
    void (*free) (qhttpclient_t *client);

    /* private variables - do not access directly */
    int socket;  /*!< socket descriptor */
    void *ssl;   /*!< will be used if SSL has been enabled at compile time */

    struct sockaddr_in addr;
    char *hostname;
    int port;

    int timeoutms;    /*< wait timeout miliseconds*/
    bool keepalive;   /*< keep-alive flag */
    char *useragent;  /*< user-agent name */

    bool connclose;   /*< response keep-alive flag for a last request */
};

/******************************************************************************
 * qdb_t - Database interface.
 * qdatabase.c
 ******************************************************************************/

/* database header files should be included before this header file. */
#ifdef _mysql_h
#define _Q_ENABLE_MYSQL  (1)
#endif /* _mysql_h */

/* types */
typedef struct qdb_s qdb_t;
typedef struct qdbresult_s qdbresult_t;

/* public functions */
extern qdb_t *qdb(const char *dbtype,
                  const char *addr, int port, const char *database,
                  const char *username, const char *password, bool autocommit);

/**
 * qdb structure
 */
struct qdb_s {
    /* capsulated member functions */
    bool (*open) (qdb_t *db);
    bool (*close) (qdb_t *db);

    int (*execute_update) (qdb_t *db, const char *query);
    int (*execute_updatef) (qdb_t *db, const char *format, ...);

    qdbresult_t *(*execute_query) (qdb_t *db, const char *query);
    qdbresult_t *(*execute_queryf) (qdb_t *db, const char *format, ...);

    bool (*begin_tran) (qdb_t *db);
    bool (*end_tran) (qdb_t *db, bool commit);
    bool (*commit) (qdb_t *db);
    bool (*rollback) (qdb_t *db);

    bool (*set_fetchtype) (qdb_t *db, bool fromdb);
    bool (*get_conn_status) (qdb_t *db);
    bool (*ping) (qdb_t *db);
    const char *(*get_error) (qdb_t *db, unsigned int *errorno);
    void (*free) (qdb_t *db);

    /* private variables - do not access directly */
    qmutex_t  qmutex;  /*!< activated if compiled with --enable-threadsafe */

    bool connected;   /*!< if opened true, if closed false */

    struct {
        char *dbtype;
        char *addr;
        int port;
        char *username;
        char *password;
        char *database;
        bool autocommit;
        bool fetchtype;
    } info;   /*!< database connection infomation */

#ifdef _Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    MYSQL  *mysql;
#endif
};

/**
 * qdbresult structure
 */
struct qdbresult_s {
    /* capsulated member functions */
    const char *(*getstr) (qdbresult_t *result, const char *field);
    const char *(*get_str_at) (qdbresult_t *result, int idx);
    int (*getint) (qdbresult_t *result, const char *field);
    int (*get_int_at) (qdbresult_t *result, int idx);
    bool (*getnext) (qdbresult_t *result);

    int (*get_cols) (qdbresult_t *result);
    int (*get_rows) (qdbresult_t *result);
    int (*get_row) (qdbresult_t *result);

    void (*free) (qdbresult_t *result);

#ifdef _Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    bool fetchtype;
    MYSQL_RES  *rs;
    MYSQL_FIELD  *fields;
    MYSQL_ROW  row;
    int cols;
    int cursor;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /*_QLIBCEXT_H */
