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
 * qlibc header file
 *
 * @file qlibc.h
 */

#ifndef _QLIBC_H
#define _QLIBC_H

#define _Q_PRGNAME "qlibc"   /*!< qlibc human readable name */
#define _Q_VERSION "2.0.0"  /*!< qlibc version number string */

#ifdef __cplusplus
//extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <netinet/in.h>

/******************************************************************************
 * COMMON DATA STRUCTURES
 ******************************************************************************/

typedef struct qmutex_t qmutex_t;    /*!< qlibc pthread mutex type*/
typedef struct qnobj_t qnobj_t;     /*!< named-object type*/
typedef struct qdlobj_t qdlobj_t;    /*!< doubly-linked-object type*/
typedef struct qdlnobj_t qdlnobj_t;  /*!< doubly-linked-named-object type*/
typedef struct qhnobj_t qhnobj_t;    /*!< hashed-named-object type*/

/**
 * qlibc pthread mutex data structure.
 */
struct qmutex_t {
    pthread_mutex_t mutex;  /*!< pthread mutex */
    pthread_t owner;        /*!< mutex owner thread id */
    int count;              /*!< recursive lock counter */
};

/**
 * named-object data structure.
 */
struct qnobj_t {
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */
};

/**
 * doubly-linked-object data structure.
 */
struct qdlobj_t {
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlobj_t *prev;     /*!< previous link */
    qdlobj_t *next;     /*!< next link */
};

/**
 * doubly-linked-named-object data structure.
 */
struct qdlnobj_t {
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlnobj_t *prev;    /*!< previous link */
    qdlnobj_t *next;    /*!< next link */
};

/**
 * hashed-named-object data structure.
 */
struct qhnobj_t {
    uint32_t hash;      /*!< FNV32 hash value of object name */
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qhnobj_t *next;     /*!< for chaining next collision object */
};

/******************************************************************************
 * Doubly Linked-list Container
 * qlist.c
 ******************************************************************************/
typedef struct qlist_t qlist_t;

/**
 * qlist container
 */
struct qlist_t {
    /* capsulated member functions */
    bool (*add_first)(qlist_t *list, const void *data, size_t size);
    bool (*add_last)(qlist_t *list, const void *data, size_t size);
    bool (*add_at)(qlist_t *list, int index, const void *data, size_t size);

    void *(*get_first)(qlist_t *list, size_t *size, bool newmem);
    void *(*get_last)(qlist_t *list, size_t *size, bool newmem);
    void *(*get_at)(qlist_t *list, int index, size_t *size, bool newmem);
    bool (*get_next)(qlist_t *list, qdlobj_t *obj, bool newmem);

    void *(*pop_first)(qlist_t *list, size_t *size);
    void *(*pop_last)(qlist_t *list, size_t *size);
    void *(*pop_at)(qlist_t *list, int index, size_t *size);

    bool (*remove_first)(qlist_t *list);
    bool (*remove_last)(qlist_t *list);
    bool (*remove_at)(qlist_t *list, int index);

    void (*reverse)(qlist_t *list);
    void (*clear)(qlist_t *list);

    size_t (*set_size)(qlist_t *list, size_t max);
    size_t (*size)(qlist_t *list);
    size_t (*datasize)(qlist_t *list);

    void *(*to_array)(qlist_t *list, size_t *size);
    char *(*to_string)(qlist_t *list);
    bool (*debug)(qlist_t *list, FILE *out);

    void (*lock)(qlist_t *list);
    void (*unlock)(qlist_t *list);

    void (*terminate)(qlist_t *list);

    /* private variables - do not access directly */
    qmutex_t qmutex;  /*!< activated if compiled with --enable-threadsafe */
    size_t num;       /*!< number of elements */
    size_t max;       /*!< maximum number of elements. 0 means no limit */
    size_t datasum;   /*!< total sum of data size, does not include name size */

    qdlobj_t *first;  /*!< first object pointer */
    qdlobj_t *last;   /*!< last object pointer */
};

/* public functions */
extern qlist_t *qlist(void); /*!< qlist constructor */

/******************************************************************************
 * Key-Value Table in Linked-list Container
 * qlisttbl.c
 ******************************************************************************/
typedef struct qlisttbl_t qlisttbl_t;

/**
 * qlisttbl container
 */
struct qlisttbl_t {
    /* capsulated member functions */
    bool (*put)(qlisttbl_t *tbl, const char *name, const void *data,
                size_t size, bool replace);
    bool (*put_first)(qlisttbl_t *tbl, const char *name, const void *data,
                      size_t size, bool replace);
    bool (*put_last)(qlisttbl_t *tbl, const char *name, const void *data,
                     size_t size, bool replace);

    bool (*put_str)(qlisttbl_t *tbl, const char *name, const char *str,
                    bool replace);
    bool (*put_strf)(qlisttbl_t *tbl, bool replace, const char *name,
                     const char *format, ...);
    bool (*put_int)(qlisttbl_t *tbl, const char *name, int num, bool replace);

    void *(*get)(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem);
    void *(*get_last)(qlisttbl_t *tbl, const char *name, size_t *size,
                      bool newmem);
    char *(*get_str)(qlisttbl_t *tbl, const char *name, bool newmem);
    int  (*get_int)(qlisttbl_t *tbl, const char *name);

    void *(*caseget)(qlisttbl_t *tbl, const char *name, size_t *size,
                     bool newmem);
    char *(*caseget_str)(qlisttbl_t *tbl, const char *name, bool newmem);
    int  (*caseget_int)(qlisttbl_t *tbl, const char *name);

    bool (*get_next)(qlisttbl_t *tbl, qdlnobj_t *obj, const char *name,
                     bool newmem);

    bool (*remove)(qlisttbl_t *tbl, const char *name);

    bool (*set_putdir)(qlisttbl_t *tbl, bool first);
    size_t (*size)(qlisttbl_t *tbl);
    void (*reverse)(qlisttbl_t *tbl);
    void (*clear)(qlisttbl_t *tbl);

    char *(*parse_str)(qlisttbl_t *tbl, const char *str);
    bool (*save)(qlisttbl_t *tbl, const char *filepath, char sepchar,
                 bool encode);
    size_t (*load)(qlisttbl_t *tbl, const char *filepath, char sepchar,
                   bool decode);
    bool (*debug)(qlisttbl_t *tbl, FILE *out);

    void (*lock)(qlisttbl_t *tbl);
    void (*unlock)(qlisttbl_t *tbl);

    void (*terminate)(qlisttbl_t *tbl);

    /* private variables - do not access directly */
    qmutex_t qmutex; /*!< activated if compiled with --enable-threadsafe */

    size_t num;   /*!< number of elements */
    bool putdir;  /*!< object appending direction for put(). false:put_last */

    qdlnobj_t *first;  /*!< first object pointer */
    qdlnobj_t *last;   /*!< last object pointer */
};

/* public functions */
extern qlisttbl_t *qlisttbl(void); /*!< qlisttbl constructor */

/******************************************************************************
 * Dynamic Hash Table Container
 * qhashtbl.c
 ******************************************************************************/
typedef struct qhashtbl_t qhashtbl_t;

/**
 * qhashtbl container
 */
struct qhashtbl_t {
    /* capsulated member functions */
    bool (*put)(qhashtbl_t *tbl, const char *name, const void *data,
                size_t size);
    bool (*put_str)(qhashtbl_t *tbl, const char *name, const char *str);
    bool (*put_strf)(qhashtbl_t *tbl, const char *name, const char *format, ...);
    bool (*put_int)(qhashtbl_t *tbl, const char *name, int num);

    void *(*get)(qhashtbl_t *tbl, const char *name, size_t *size, bool newmem);
    char *(*get_str)(qhashtbl_t *tbl, const char *name, bool newmem);
    int (*get_int)(qhashtbl_t *tbl, const char *name);

    bool (*get_next)(qhashtbl_t *tbl, qhnobj_t *obj, bool newmem);

    bool (*remove)(qhashtbl_t *tbl, const char *name);

    size_t (*size)(qhashtbl_t *tbl);
    void (*clear)(qhashtbl_t *tbl);
    bool (*debug)(qhashtbl_t *tbl, FILE *out);

    void (*lock)(qhashtbl_t *tbl);
    void (*unlock)(qhashtbl_t *tbl);

    void (*terminate)(qhashtbl_t *tbl);

    /* private variables - do not access directly */
    qmutex_t qmutex;    /*!< activated if compiled with --enable-threadsafe */
    size_t num;         /*!< number of objects in this table */
    size_t range;       /*!< hash range, vertical number of slots */
    qhnobj_t **slots;   /*!< slot pointer container */
};

/* public functions */
extern qhashtbl_t *qhashtbl(size_t range);  /*!< qhashtbl constructor */

/******************************************************************************
 * Static Hash Table Container - works in fixed size memory
 * qhasharr.c
 ******************************************************************************/
#define _Q_HASHARR_KEYSIZE (16)    /*!< knob for maximum key size. */
#define _Q_HASHARR_VALUESIZE (32)  /*!< knob for maximum data size in a slot. */
typedef struct qhasharr_t qhasharr_t;

/**
 * Q_HASHARR constructor and memory size calculator.
 */
extern qhasharr_t *qhasharr(void *memory, size_t memsize);
extern size_t qhasharr_calculate_memsize(int max);

/**
 * qhasharr internal data slot structure
 */
struct _Q_HASHARR_SLOT {
    short  count;   /*!< hash collision counter. 0 indicates empty slot, -1 is used for collision resolution, -2 is used for indicating linked block */
    uint32_t  hash;   /*!< key hash. we use FNV32 */

    uint8_t  size;   /*!< value size in this slot*/
    int link;   /*!< next link */

    union {
        /*!< key/value data */
        struct _Q_HASHARR_SLOT_KEYVAL {
            unsigned char value[_Q_HASHARR_VALUESIZE]; /*!< value */

            char key[_Q_HASHARR_KEYSIZE]; /*!< key string, it can be truncated */
            uint16_t  keylen;   /*!< original key length */
            unsigned char keymd5[16];   /*!< md5 hash of the key */
        } pair;

        /*!< extended data block, used only when the count value is -2 */
        struct _Q_HASHARR_SLOT_EXT {
            unsigned char value[sizeof(struct _Q_HASHARR_SLOT_KEYVAL)]; /*!< value */
        } ext;
    } data;
};

/**
 * qhasharr container
 */
struct qhasharr_t {
    /* capsulated member functions */
    bool (*put)(qhasharr_t *tbl, const char *key, const void *value, size_t size);
    bool (*put_str)(qhasharr_t *tbl, const char *key, const char *str);
    bool (*put_strf)(qhasharr_t *tbl, const char *key, const char *format, ...);
    bool (*put_int)(qhasharr_t *tbl, const char *key, int num);

    void *(*get)(qhasharr_t *tbl, const char *key, size_t *size);
    char *(*get_str)(qhasharr_t *tbl, const char *key);
    int  (*get_int)(qhasharr_t *tbl, const char *key);
    bool (*get_next)(qhasharr_t *tbl, qnobj_t *obj, int *idx);

    bool (*remove)(qhasharr_t *tbl, const char *key);

    int  (*size)(qhasharr_t *tbl, int *maxslots, int *usedslots);
    void (*clear)(qhasharr_t *tbl);
    bool (*debug)(qhasharr_t *tbl, FILE *out);

    /* private variables - do not access directly */
    qmutex_t  qmutex; /*!< activated if compiled with --enable-threadsafe */
    int maxslots; /*!< number of maximum slots */
    int usedslots; /*!< number of used slots */
    int num; /*!< number of stored keys */
    struct _Q_HASHARR_SLOT  *slots; /*!< data area pointer */
};

/* -------------------------------------------------------------------------
 * Q_VECTOR Container - vector implementation.
 * qVector.c
 * ------------------------------------------------------------------------- */

/**
 * Q_VECTOR types and definitions.
 */
typedef struct _Q_VECTOR  Q_VECTOR;

/**
 * Q_VECTOR constructor.
 */
extern Q_VECTOR *qVector(void);

/**
 * Q_VECTOR container details.
 */
struct _Q_VECTOR {
    /* capsulated member functions */
    bool (*add) (Q_VECTOR *vector, const void *data, size_t size);
    bool (*addStr) (Q_VECTOR *vector, const char *str);
    bool (*addStrf) (Q_VECTOR *vector, const char *format, ...);

    void *(*toArray) (Q_VECTOR *vector, size_t *size);
    char *(*toString) (Q_VECTOR *vector);

    size_t (*size) (Q_VECTOR *vector);
    size_t (*datasize) (Q_VECTOR *vector);
    void (*clear) (Q_VECTOR *vector);
    bool (*debug) (Q_VECTOR *vector, FILE *out);

    void (*free) (Q_VECTOR *vector);

    /* private variables - do not access directly */
    qlist_t *list; /*!< data container */
};

/* -------------------------------------------------------------------------
 * Q_QUEUE Container - queue implementation.
 * qHasharr.c
 * ------------------------------------------------------------------------- */
typedef struct _Q_QUEUE Q_QUEUE;
/*
 * qQueue.c
 */
extern Q_QUEUE *qQueue();

/**
 * Structure for array-based circular-queue data structure.
 */
struct _Q_QUEUE {
    /* capsulated member functions */
    size_t (*setSize) (Q_QUEUE *stack, size_t max);

    bool (*push) (Q_QUEUE *stack, const void *data, size_t size);
    bool (*pushStr) (Q_QUEUE *stack, const char *str);
    bool (*pushInt) (Q_QUEUE *stack, int num);

    void *(*pop) (Q_QUEUE *stack, size_t *size);
    char *(*popStr) (Q_QUEUE *stack);
    int (*popInt) (Q_QUEUE *stack);
    void *(*popAt) (Q_QUEUE *stack, int index, size_t *size);

    void *(*get) (Q_QUEUE *stack, size_t *size, bool newmem);
    char *(*getStr) (Q_QUEUE *stack);
    int (*getInt) (Q_QUEUE *stack);
    void *(*getAt) (Q_QUEUE *stack, int index, size_t *size, bool newmem);

    size_t (*size) (Q_QUEUE *stack);
    void (*clear) (Q_QUEUE *stack);
    bool (*debug) (Q_QUEUE *stack, FILE *out);
    void (*free) (Q_QUEUE *stack);

    /* private variables - do not access directly */
    qlist_t  *list; /*!< data container */
};

/* -------------------------------------------------------------------------
 * LIFO stack container
 * qHasharr.c
 * ------------------------------------------------------------------------- */
typedef struct qstack_tr qstack_t;
/*
 * qQueue.c
 */
extern qstack_t *qStack();

/**
 * Structure for array-based circular-queue data structure.
 */
struct qstack_tr {
    /* capsulated member functions */
    size_t (*set_size) (qstack_t *stack, size_t max);

    bool (*push) (qstack_t *stack, const void *data, size_t size);
    bool (*push_str) (qstack_t *stack, const char *str);
    bool (*push_int) (qstack_t *stack, int num);

    void *(*pop) (qstack_t *stack, size_t *size);
    char *(*popStr) (qstack_t *stack);
    int (*popInt) (qstack_t *stack);
    void *(*popAt) (qstack_t *stack, int index, size_t *size);

    void *(*get) (qstack_t *stack, size_t *size, bool newmem);
    char *(*getStr) (qstack_t *stack);
    int (*getInt) (qstack_t *stack);
    void *(*getAt) (qstack_t *stack, int index, size_t *size, bool newmem);

    size_t (*size) (qstack_t *stack);
    void (*clear) (qstack_t *stack);
    bool (*debug) (qstack_t *stack, FILE *out);
    void (*free) (qstack_t *stack);

    /* private variables - do not access directly */
    qlist_t  *list; /*!< data container */
};

/* =========================================================================
 *
 * UTILITIES SECTION
 *
 * ========================================================================= */

/*
 * qCount.c
 */
extern int qCountRead(const char *filepath);
extern bool qCountSave(const char *filepath, int number);
extern int qCountUpdate(const char *filepath, int number);

/*
 * qEncode.c
 */
extern qlisttbl_t *qParseQueries(qlisttbl_t *tbl, const char *query, char equalchar, char sepchar, int *count);
extern char *qUrlEncode(const void *bin, size_t size);
extern size_t qUrlDecode(char *str);
extern char *qBase64Encode(const void *bin, size_t size);
extern size_t qBase64Decode(char *str);
extern char *qHexEncode(const void *bin, size_t size);
extern size_t qHexDecode(char *str);

/*
 * qFile.c
 */
extern bool qFileLock(int fd);
extern bool qFileUnlock(int fd);
extern bool qFileExist(const char *filepath);
extern void *qFileLoad(const char *filepath, size_t *nbytes);
extern void *qFileRead(FILE *fp, size_t *nbytes);
extern ssize_t qFileSave(const char *filepath, const void *buf, size_t size, bool append);
extern bool qFileMkdir(const char *dirpath, mode_t mode, bool recursive);

extern char *qFileGetName(const char *filepath);
extern char *qFileGetDir(const char *filepath);
extern char *qFileGetExt(const char *filepath);
extern off_t qFileGetSize(const char *filepath);

extern bool qFileCheckPath(const char *path);
extern char *qFileCorrectPath(char *path);
extern char *qFileGetAbsPath(char *buf, size_t bufsize, const char *path);

/*
 * qHash.c
 */
extern unsigned char *qHashMd5(const void *data, size_t nbytes);
extern char *qHashMd5Str(const void *data, size_t nbytes);
extern char *qHashMd5File(const char *filepath, size_t *nbytes);
extern uint32_t  qHashFnv32(const void *data, size_t nbytes);
extern uint64_t  qHashFnv64(const void *data, size_t nbytes);

/*
 * qIo.c
 */
extern int qIoWaitReadable(int fd, int timeoutms);
extern int qIoWaitWritable(int fd, int timeoutms);
extern ssize_t qIoRead(int fd, void *buf, size_t nbytes, int timeoutms);
extern ssize_t qIoWrite(int fd, const void *data, size_t nbytes, int timeoutms);
extern off_t qIoSend(int outfd, int infd, off_t nbytes, int timeoutms);
extern ssize_t qIoGets(int fd, char *buf, size_t bufsize, int timeoutms);
extern ssize_t qIoPuts(int fd, const char *str, int timeoutms);
extern ssize_t qIoPrintf(int fd, int timeoutms, const char *format, ...);

/*
 * qLibc.c
 */
extern const char *qlibc_version(void);
extern bool qlibc_is_threadsafe(void);
extern bool qlibc_is_lfs(void);

/*
 * qSocket.c
 */
extern int qSocketOpen(const char *hostname, int port, int timeoutms);
extern bool qSocketClose(int sockfd, int timeoutms);
extern bool qSocketGetAddr(struct sockaddr_in *addr, const char *hostname, int port);

/*
 * qString.c
 */
extern char *qStrTrim(char *str);
extern char *qStrTrimTail(char *str);
extern char *qStrUnchar(char *str, char head, char tail);
extern char *qStrReplace(const char *mode, char *srcstr, const char *tokstr, const char *word);
extern char *qStrCpy(char *dst, size_t size, const char *src);
extern char *qStrCpyn(char *dst, size_t size, const char *src, size_t nbytes);
extern char *qStrDupf(const char *format, ...);
extern char *qStrGets(char *buf, size_t size, char **offset);
extern char *qStrUpper(char *str);
extern char *qStrLower(char *str);
extern char *qStrRev(char *str);
extern char *qStrTok(char *str, const char *delimiters, char *retstop, int *offset);
extern qlist_t *qStrTokenizer(const char *str, const char *delimiters);
extern char *qStrCommaNumber(int number);
extern char *qStrCatf(char *str, const char *format, ...);
extern char *qStrDupBetween(const char *str, const char *start, const char *end);
extern char *qStrUnique(const char *seed);
extern bool qStrTest(int (*testfunc)(int), const char *str);
extern bool qStrIsEmail(const char *email);
extern bool qStrIsIpv4Addr(const char *str);
extern char *qStrConvEncoding(const char *fromstr, const char *fromcode, const char *tocode, float mag);

/*
 * qSystem.c
 */
extern const char *qSysGetEnv(const char *envname, const char *nullstr);
extern char *qSysCmd(const char *cmd);
extern bool qSysGetIp(char *buf, size_t bufsize);

/*
 * qTime.c
 */
extern char *qTimeGetLocalStrf(char *buf, int size, time_t utctime, const char *format);
extern char *qTimeGetLocalStr(time_t utctime);
extern const char *qTimeGetLocalStaticStr(time_t utctime);
extern char *qTimeGetGmtStrf(char *buf, int size, time_t utctime, const char *format);
extern char *qTimeGetGmtStr(time_t utctime);
extern const char *qTimeGetGmtStaticStr(time_t utctime);
extern time_t  qTimeParseGmtStr(const char *gmtstr);


/* =========================================================================
 *
 * IPC SECTION
 *
 * ========================================================================= */

/*
 * qSem.c
 */
extern int qSemInit(const char *keyfile, int keyid, int nsems, bool recreate);
extern int qSemGetId(const char *keyfile, int keyid);
extern bool qSemEnter(int semid, int semno);
extern bool qSemEnterNowait(int semid, int semno);
extern bool qSemEnterForce(int semid, int semno, int maxwaitms, bool *forceflag);
extern bool qSemLeave(int semid, int semno);
extern bool qSemCheck(int semid, int semno);
extern bool qSemFree(int semid);

/*
 * qShm.c
 */
extern int qShmInit(const char *keyfile, int keyid, size_t size, bool recreate);
extern int qShmGetId(const char *keyfile, int keyid);
extern void *qShmGet(int shmid);
extern bool qShmFree(int shmid);

/* =========================================================================
 *
 * EXTENSIONS SECTION
 *
 * ========================================================================= */

/* -------------------------------------------------------------------------
 * Configuration Parser.
 * qConfig.c
 * ------------------------------------------------------------------------- */
extern qlisttbl_t *qConfigParseFile(qlisttbl_t *tbl, const char *filepath, char sepchar);
extern qlisttbl_t *qConfigParseStr(qlisttbl_t *tbl, const char *str, char sepchar);

/* -------------------------------------------------------------------------
 * Q_LOG - Rotating file logger.
 * qLog.c
 * ------------------------------------------------------------------------- */

/**
 * Q_LOG types and definitions.
 */
typedef struct _Q_LOG  Q_LOG;

/**
 * Q_LOG constructor.
 */
extern Q_LOG *qLog(const char *filepathfmt, mode_t mode, int rotateinterval, bool flush);

/**
 * Q_LOG details.
 */
struct _Q_LOG {
    /* capsulated member functions */
    bool (*write) (Q_LOG *log, const char *str);
    bool (*writef) (Q_LOG *log, const char *format, ...);
    bool (*duplicate) (Q_LOG *log, FILE *outfp, bool flush);
    bool (*flush) (Q_LOG *log);
    bool (*free) (Q_LOG *log);

    /* private variables - do not access directly */
    qmutex_t  qmutex; /*!< activated if compiled with --enable-threadsafe */

    char filepathfmt[PATH_MAX]; /*!< file file naming format like /somepath/daily-%Y%m%d.log */
    char filepath[PATH_MAX]; /*!< generated system path of log file */
    FILE *fp;   /*!< file pointer of logpath */
    mode_t  mode;   /*!< file mode */
    int rotateinterval; /*!< log file will be rotate in this interval seconds */
    int nextrotate; /*!< next rotate universal time, seconds */
    bool logflush; /*!< flag for immediate flushing */

    FILE *outfp;   /*!< stream pointer for duplication */
    bool outflush; /*!< flag for immediate flushing for duplicated stream */
};

/* -------------------------------------------------------------------------
 * Q_HTTPCLIENT - HTTP client implementation.
 * qHttpClient.c
 * ------------------------------------------------------------------------- */

/**
 * Q_HTTPCLIENT types and definitions.
 */
typedef struct _Q_HTTPCLIENT  Q_HTTPCLIENT;

/**
 * Q_HTTPCLIENT constructor.
 */
extern Q_HTTPCLIENT *qHttpClient(const char *hostname, int port);

/**
 * Q_HTTPCLIENT details.
 */
struct _Q_HTTPCLIENT {
    /* capsulated member functions */
    bool (*setSsl) (Q_HTTPCLIENT *client);
    void (*setTimeout) (Q_HTTPCLIENT *client, int timeoutms);
    void (*setKeepalive) (Q_HTTPCLIENT *client, bool keepalive);
    void (*setUseragent) (Q_HTTPCLIENT *client, const char *useragent);

    bool (*open) (Q_HTTPCLIENT *client);

    bool (*head) (Q_HTTPCLIENT *client, const char *uri, int *rescode, qlisttbl_t *reqheaders, qlisttbl_t *resheaders);
    bool (*get) (Q_HTTPCLIENT *client, const char *uri, int fd, off_t *savesize, int *rescode, qlisttbl_t *reqheaders, qlisttbl_t *resheaders, bool (*callback)(void *userdata, off_t recvbytes), void *userdata);
    bool (*put) (Q_HTTPCLIENT *client, const char *uri, int fd, off_t length, int *retcode, qlisttbl_t *userheaders, qlisttbl_t *resheaders, bool (*callback)(void *userdata, off_t sentbytes), void *userdata);
    void *(*cmd) (Q_HTTPCLIENT *client, const char *method, const char *uri, void *data, size_t size, int *rescode, size_t *contentslength, qlisttbl_t *reqheaders, qlisttbl_t *resheaders);

    bool (*sendRequest) (Q_HTTPCLIENT *client, const char *method, const char *uri, qlisttbl_t *reqheaders);
    int (*readResponse) (Q_HTTPCLIENT *client, qlisttbl_t *resheaders, off_t *contentlength);

    ssize_t (*gets) (Q_HTTPCLIENT *client, char *buf, size_t bufsize);
    ssize_t (*read) (Q_HTTPCLIENT *client, void *buf, size_t nbytes);
    ssize_t (*write) (Q_HTTPCLIENT *client, const void *buf, size_t nbytes);
    off_t (*recvfile) (Q_HTTPCLIENT *client, int fd, off_t nbytes);
    off_t (*sendfile) (Q_HTTPCLIENT *client, int fd, off_t nbytes);

    bool (*close) (Q_HTTPCLIENT *client);
    void (*free) (Q_HTTPCLIENT *client);

    /* private variables - do not access directly */
    int socket;   /*!< socket descriptor */
    void *ssl;   /*!< will be used if SSL has been enabled at compile time */

    struct sockaddr_in addr;
    char *hostname;
    int port;

    int timeoutms; /*< wait timeout miliseconds*/
    bool keepalive; /*< keep-alive flag */
    char *useragent; /*< user-agent name */

    bool connclose; /*< response keep-alive flag for a last request */
};

/* -------------------------------------------------------------------------
 * Q_DB - Database interface.
 * qDatabase.c
 * ------------------------------------------------------------------------- */

/**
 * Q_DB & Q_DBRESULT types and definitions.
 */
typedef struct _Q_DB  Q_DB;
typedef struct _Q_DBRESULT  Q_DBRESULT;

/* Database Support*/
#ifdef _mysql_h
#define _Q_ENABLE_MYSQL  (1)
#endif /* _mysql_h */

/**
 * Q_DB constructor.
 */
extern Q_DB *qDb(const char *dbtype, const char *addr, int port, const char *database, const char *username, const char *password, bool autocommit);

/**
 * Q_DB details.
 */
struct _Q_DB {
    /* capsulated member functions */
    bool (*open) (Q_DB *db);
    bool (*close) (Q_DB *db);

    int (*executeUpdate) (Q_DB *db, const char *query);
    int (*executeUpdatef) (Q_DB *db, const char *format, ...);

    Q_DBRESULT *(*executeQuery) (Q_DB *db, const char *query);
    Q_DBRESULT *(*executeQueryf) (Q_DB *db, const char *format, ...);

    bool (*beginTran) (Q_DB *db);
    bool (*endTran) (Q_DB *db, bool commit);
    bool (*commit) (Q_DB *db);
    bool (*rollback) (Q_DB *db);

    bool (*setFetchType) (Q_DB *db, bool use);
    bool (*getConnStatus) (Q_DB *db);
    bool (*ping) (Q_DB *db);
    const char *(*getError) (Q_DB *db, unsigned int *errorno);
    bool (*free) (Q_DB *db);

    /* private variables - do not access directly */
    qmutex_t  qmutex; /*!< activated if compiled with --enable-threadsafe */

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

    /* private variables for mysql database */
#ifdef _Q_ENABLE_MYSQL
    MYSQL  *mysql;
#endif
};

/**
 * Q_DBRESULT details.
 */
struct _Q_DBRESULT {
    /* capsulated member functions */
    const char *(*getStr) (Q_DBRESULT *result, const char *field);
    const char *(*getStrAt) (Q_DBRESULT *result, int idx);
    int (*getInt) (Q_DBRESULT *result, const char *field);
    int (*getIntAt) (Q_DBRESULT *result, int idx);
    bool (*getNext) (Q_DBRESULT *result);

    int (*getCols) (Q_DBRESULT *result);
    int (*getRows) (Q_DBRESULT *result);
    int (*getRow) (Q_DBRESULT *result);

    bool (*free) (Q_DBRESULT *result);

    /* private variables - do not access directly */

    /* private variables for mysql database */
#ifdef _Q_ENABLE_MYSQL
    bool fetchtype;
    MYSQL_RES  *rs;
    MYSQL_FIELD  *fields;
    MYSQL_ROW  row;
    int cols;
    int cursor;
#endif
};

#ifdef __cplusplus
//}
#endif

#endif /*_QLIBC_H */
