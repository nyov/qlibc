/*
 * qLibc - http://www.qdecoder.org
 *
 * Copyright 2010 qDecoder Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY QDECODER PROJECT ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE QDECODER PROJECT BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

/**
 * qlibc Header file
 *
 * @file	qlibc.h
 */

#ifndef _QLIBC_H
#define _QLIBC_H

#define _Q_PRGNAME			"qlibc"
#define _Q_VERSION			"1.0.4"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <netinet/in.h>

/******************************************************************************
 * COMMON STRUCTURE DEFINITIONS
 *****************************************************************************/

typedef struct _Q_MUTEX_T	Q_MUTEX_T;
typedef struct _Q_NOBJ_T	Q_NOBJ_T;
typedef struct _Q_DLOBJ_T	Q_DLOBJ_T;
typedef struct _Q_NDLOBJ_T	Q_NDLOBJ_T;
typedef struct _Q_NHLOBJ_T	Q_NHLOBJ_T;

/**
 * Variable type for mutex lock management
 */
struct _Q_MUTEX_T {
	pthread_mutex_t	mutex;		/*!< pthread mutex */
	pthread_t	owner;		/*!< mutex owner thread id */
	int		count;		/*!< recursive lock counter */
};

/**
 * Variable type for Named-OBJect.
 */
struct _Q_NOBJ_T {
	char*		name;		/*!< object name */
	void*		data;		/*!< data object */
	size_t		size;		/*!< object size */
};

/**
 * Variable type for Doubly-Linked-OBJect.
 */
struct _Q_DLOBJ_T {
	void*		data;		/*!< data object */
	size_t		size;		/*!< object size */

	Q_DLOBJ_T*	prev;		/*!< previous link pointer */
	Q_DLOBJ_T*	next;		/*!< next link pointer */
};

/**
 * Variable type for Named-Doubly-Linked-OBJect.
 */
struct _Q_NDLOBJ_T {
	char*		name;		/*!< object name */
	void*		data;		/*!< data object */
	size_t		size;		/*!< object size */

	Q_NDLOBJ_T*	prev;		/*!< previous link pointer */
	Q_NDLOBJ_T*	next;		/*!< next link pointer */
};

/**
 * Variable type for Named-Hash-Linked-OBJect.
 */
struct _Q_NHLOBJ_T {
	uint32_t	hash;		/*!< FNV32 hash value of object name */
	char*		name;		/*!< object name */
	void*		data;		/*!< data object */
	size_t		size;		/*!< object size */

	Q_NHLOBJ_T*	next;		/*!< next link pointer */
};

/* =========================================================================
 *
 * CONTAINER SECTION
 *
 * ========================================================================= */

/* -------------------------------------------------------------------------
 * Q_LIST Container - "Linked-List" implementation.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * qList.c
 * ------------------------------------------------------------------------- */

/**
 * Q_LIST types and definitions.
 */
typedef	struct _Q_LIST		Q_LIST;

/**
 * Q_LIST constructor.
 */
extern	Q_LIST*		qList(void);	/*!< Q_LIST constructor */

/**
 * Q_LIST container details.
 */
struct _Q_LIST {
	/* public member methods */
	size_t		(*setSize)	(Q_LIST *list, size_t max);

	bool		(*addFirst)	(Q_LIST *list, const void *data, size_t size);
	bool		(*addLast)	(Q_LIST *list, const void *data, size_t size);
	bool		(*addAt)	(Q_LIST *list, int index, const void *data, size_t size);

	void*		(*getFirst)	(Q_LIST *list, size_t *size, bool newmem);
	void*		(*getLast)	(Q_LIST *list, size_t *size, bool newmem);
	void*		(*getAt)	(Q_LIST *list, int index, size_t *size, bool newmem);
	bool		(*getNext)	(Q_LIST *list, Q_DLOBJ_T *obj, bool newmem);

	void*		(*popFirst)	(Q_LIST *list, size_t *size);
	void*		(*popLast)	(Q_LIST *list, size_t *size);
	void*		(*popAt)	(Q_LIST *list, int index, size_t *size);

	bool		(*removeFirst)	(Q_LIST *list);
	bool		(*removeLast)	(Q_LIST *list);
	bool		(*removeAt)	(Q_LIST *list, int index);

	size_t		(*size)		(Q_LIST *list);
	size_t		(*datasize)	(Q_LIST *list);
	void		(*reverse)	(Q_LIST *list);
	void		(*clear)	(Q_LIST *list);

	void*		(*toArray)	(Q_LIST *list, size_t *size);
	char*		(*toString)	(Q_LIST *list);
	bool		(*debug)	(Q_LIST *list, FILE *out);

	void		(*lock)		(Q_LIST *list);
	void		(*unlock)	(Q_LIST *list);

	void		(*free)		(Q_LIST *list);

	/* private variables */
	Q_MUTEX_T	qmutex;		/*!< activated if compiled with --enable-threadsafe option */
	size_t		num;		/*!< number of elements */
	size_t		max;		/*!< maximum number of elements. 0 means no limit */
	size_t		datasum;	/*!< total sum of data size, does not include name size */

	Q_DLOBJ_T*	first;		/*!< first object pointer */
	Q_DLOBJ_T*	last;		/*!< last object pointer */
};

/* -------------------------------------------------------------------------
 * Q_LISTTBL Container - "Linked-List-Table" implementation.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * qListtbl.c
 * ----------------------------------------------------------------------- */

/**
 * Q_LISTTBL types and definitions.
 */
typedef struct _Q_LISTTBL	Q_LISTTBL;

/**
 * Q_LISTTBL constructor.
 */
extern	Q_LISTTBL*		qListtbl(void);	/*!< Q_LISTTBL constructor */

/**
 * Q_LISTTBL container details.
 */
struct _Q_LISTTBL {
	/* public member methods */
	bool		(*setPutDirection)	(Q_LISTTBL *tbl, bool first);

	bool		(*put)		(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool replace);
	bool		(*putFirst)	(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool replace);
	bool		(*putLast)	(Q_LISTTBL *tbl, const char *name, const void *data, size_t size, bool replace);

	bool		(*putStr)	(Q_LISTTBL *tbl, const char *name, const char *str, bool replace);
	bool		(*putStrf)	(Q_LISTTBL *tbl, bool replace, const char *name, const char *format, ...);
	bool		(*putInt)	(Q_LISTTBL *tbl, const char *name, int num, bool replace);

	void*		(*get)		(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem);
	void*		(*getLast)	(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem);
	char*		(*getStr)	(Q_LISTTBL *tbl, const char *name, bool newmem);
	int		(*getInt)	(Q_LISTTBL *tbl, const char *name);

	void*		(*getCase)	(Q_LISTTBL *tbl, const char *name, size_t *size, bool newmem);
	char*		(*getCaseStr)	(Q_LISTTBL *tbl, const char *name, bool newmem);
	int		(*getCaseInt)	(Q_LISTTBL *tbl, const char *name);

	bool		(*getNext)	(Q_LISTTBL *tbl, Q_NDLOBJ_T *obj, const char *name, bool newmem);

	bool		(*remove)	(Q_LISTTBL *tbl, const char *name);

	size_t 		(*size)		(Q_LISTTBL *tbl);
	void		(*reverse)	(Q_LISTTBL *tbl);
	void		(*clear)	(Q_LISTTBL *tbl);

	char*		(*parseStr)	(Q_LISTTBL *tbl, const char *str);
	bool		(*save)		(Q_LISTTBL *tbl, const char *filepath, char sepchar, bool encode);
	size_t		(*load)		(Q_LISTTBL *tbl, const char *filepath, char sepchar, bool decode);
	bool		(*debug)	(Q_LISTTBL *tbl, FILE *out);

	void		(*lock)		(Q_LISTTBL *tbl);
	void		(*unlock)	(Q_LISTTBL *tbl);

	void		(*free)		(Q_LISTTBL *tbl);

	/* private variables */
	Q_MUTEX_T	qmutex;		/*!< activated if compiled with --enable-threadsafe option */

	size_t		num;		/*!< number of elements */
	bool		putdir;		/*!< object adding direction. true for adding from the beginning. default is false. */

	Q_NDLOBJ_T*	first;		/*!< first object pointer */
	Q_NDLOBJ_T*	last;		/*!< last object pointer */
};

/* -------------------------------------------------------------------------
 * Q_HASHTBL Container - dynamic hash table implementation.
 * qHashtbl.c
 * ------------------------------------------------------------------------- */

/**
 * Q_HASHTBL types and definitions.
 */
typedef struct _Q_HASHTBL	Q_HASHTBL;

/**
 * Q_HASHTBL constructor.
 */
extern	Q_HASHTBL*	qHashtbl(size_t range);	/*!< Q_HASHTBL constructor */

/**
 * Q_HASHTBL container details.
 */
struct _Q_HASHTBL {
	/* public member methods */
	bool		(*put)		(Q_HASHTBL *tbl, const char *name, const void *data, size_t size);
	bool		(*putStr)	(Q_HASHTBL *tbl, const char *name, const char *str);
	bool		(*putStrf)	(Q_HASHTBL *tbl, const char *name, const char *format, ...);
	bool		(*putInt)	(Q_HASHTBL *tbl, const char *name, int num);

	void*		(*get)		(Q_HASHTBL *tbl, const char *name, size_t *size, bool newmem);
	char*		(*getStr)	(Q_HASHTBL *tbl, const char *name, bool newmem);
	int		(*getInt)	(Q_HASHTBL *tbl, const char *name);

	bool		(*getNext)	(Q_HASHTBL *tbl, Q_NHLOBJ_T *obj, bool newmem);

	bool		(*remove)	(Q_HASHTBL *tbl, const char *name);

	size_t 		(*size)		(Q_HASHTBL *tbl);
	void		(*clear)	(Q_HASHTBL *tbl);
	bool		(*debug)	(Q_HASHTBL *tbl, FILE *out);

	void		(*lock)		(Q_HASHTBL *tbl);
	void		(*unlock)	(Q_HASHTBL *tbl);

	void		(*free)		(Q_HASHTBL *tbl);

	/* private variables */
	Q_MUTEX_T	qmutex;		/*!< activated if compiled with --enable-threadsafe option */

	size_t		num;		/*!< number of objects in this table */
	size_t		range;		/*!< hash range, vertical number of slots */
	Q_NHLOBJ_T**	slots;		/*!< slot pointer container */
};

/* -------------------------------------------------------------------------
 * Q_HASHARR Container - static hash table implementation.
 * qHasharr.c
 * ------------------------------------------------------------------------- */

/**
 * Q_HASHARR types and definitions.
 */
#define _Q_HASHARR_KEYSIZE		(16)		/*!< maximum key size. */
#define _Q_HASHARR_VALUESIZE		(32)		/*!< maximum value size in a slot. */
typedef struct _Q_HASHARR		Q_HASHARR;

/**
 * Q_HASHARR constructor and memory size calculator.
 */
extern	Q_HASHARR*	qHasharr(void *memory, size_t memsize);	/*!< Q_HASHARR constructor */
extern	size_t		qHasharrSize(int max);

/**
 * Q_HASHARR slot details.
 */
struct _Q_HASHARR_SLOT {
	short		count;				/*!< hash collision counter. 0 indicates empty slot, -1 is used for collision resolution, -2 is used for indicating linked block */
	uint32_t	hash;				/*!< key hash. we use FNV32 */

	uint8_t		size;				/*!< value size in this slot*/
	int		link;				/*!< next link */

	union {
		/*!< key/value data */
		struct _Q_HASHARR_SLOT_KEYVAL {
			unsigned char	value[_Q_HASHARR_VALUESIZE];	/*!< value */

			char		key[_Q_HASHARR_KEYSIZE];	/*!< key string, it can be truncated */
			uint16_t	keylen;				/*!< original key length */
			unsigned char	keymd5[16];			/*!< md5 hash of the key */
		} pair;

		/*!< extended data block, only used when the count value is -2 */
		struct _Q_HASHARR_SLOT_EXT {
			unsigned char value[sizeof(struct _Q_HASHARR_SLOT_KEYVAL)];	/*!< value */
		} ext;
	} data;
};

/**
 * Q_HASHARR container details.
 */
struct _Q_HASHARR {
	/* public member methods */
	bool		(*put)		(Q_HASHARR *tbl, const char *key, const void *value, size_t size);
	bool		(*putStr)	(Q_HASHARR *tbl, const char *key, const char *str);
	bool		(*putStrf)	(Q_HASHARR *tbl, const char *key, const char *format, ...);
	bool		(*putInt)	(Q_HASHARR *tbl, const char *key, int num);

	void*		(*get)		(Q_HASHARR *tbl, const char *key, size_t *size);
	char*		(*getStr)	(Q_HASHARR *tbl, const char *key);
	int		(*getInt)	(Q_HASHARR *tbl, const char *key);
	bool		(*getNext)	(Q_HASHARR *tbl, Q_NOBJ_T *obj, int *idx);

	bool		(*remove)	(Q_HASHARR *tbl, const char *key);

	int		(*size)		(Q_HASHARR *tbl, int *maxslots, int *usedslots);
	void		(*clear)	(Q_HASHARR *tbl);
	bool		(*debug)	(Q_HASHARR *tbl, FILE *out);

	/* private variables */
	Q_MUTEX_T	qmutex;		/*!< activated if compiled with --enable-threadsafe option */

	int		maxslots;	/*!< number of maximum slots */
	int		usedslots;	/*!< number of used slots */
	int		num;		/*!< number of stored keys */

	struct _Q_HASHARR_SLOT	*slots;	/*!< data area pointer */
};

/* -------------------------------------------------------------------------
 * Q_VECTOR Container - vector implementation.
 * qVector.c
 * ------------------------------------------------------------------------- */

/**
 * Q_VECTOR types and definitions.
 */
typedef struct _Q_VECTOR	Q_VECTOR;

/**
 * Q_VECTOR constructor.
 */
extern	Q_VECTOR*	qVector(void);

/**
 * Q_VECTOR container details.
 */
struct _Q_VECTOR {
	/* public member methods */
	bool		(*add)		(Q_VECTOR *vector, const void *data, size_t size);
	bool		(*addStr)	(Q_VECTOR *vector, const char *str);
	bool		(*addStrf)	(Q_VECTOR *vector, const char *format, ...);

	void*		(*toArray)	(Q_VECTOR *vector, size_t *size);
	char*		(*toString)	(Q_VECTOR *vector);

	size_t		(*size)		(Q_VECTOR *vector);
	size_t		(*datasize)	(Q_VECTOR *vector);
	void		(*clear)	(Q_VECTOR *vector);
	bool		(*debug)	(Q_VECTOR *vector, FILE *out);

	void		(*free)		(Q_VECTOR *vector);

	/* private variables */
	Q_LIST		*list;		/*!< data container */
};

/* -------------------------------------------------------------------------
 * Q_QUEUE Container -  queue implementation.
 * qHasharr.c
 * ------------------------------------------------------------------------- */
typedef struct _Q_QUEUE		Q_QUEUE;
/*
 * qQueue.c
 */
extern	Q_QUEUE*	qQueue();

/**
 * Structure for array-based circular-queue data structure.
 */
struct _Q_QUEUE {
	/* public member methods */
	size_t		(*setSize)	(Q_QUEUE *stack, size_t max);

	bool		(*push)		(Q_QUEUE *stack, const void *data, size_t size);
	bool		(*pushStr)	(Q_QUEUE *stack, const char *str);
	bool		(*pushInt)	(Q_QUEUE *stack, int num);

	void*		(*pop)		(Q_QUEUE *stack, size_t *size);
	char*		(*popStr)	(Q_QUEUE *stack);
	int		(*popInt)	(Q_QUEUE *stack);
	void*		(*popAt)	(Q_QUEUE *stack, int index, size_t *size);

	void*		(*get)		(Q_QUEUE *stack, size_t *size, bool newmem);
	char*		(*getStr)	(Q_QUEUE *stack);
	int		(*getInt)	(Q_QUEUE *stack);
	void*		(*getAt)	(Q_QUEUE *stack, int index, size_t *size, bool newmem);

	size_t		(*size)		(Q_QUEUE *stack);
	void		(*clear)	(Q_QUEUE *stack);
	bool		(*debug)	(Q_QUEUE *stack, FILE *out);
	void		(*free)		(Q_QUEUE *stack);

	/* private variables */
	Q_LIST		*list;		/*!< data container */
};

/* -------------------------------------------------------------------------
 * Q_STACK Container -  stack implementation.
 * qHasharr.c
 * ------------------------------------------------------------------------- */
typedef struct _Q_STACK		Q_STACK;
/*
 * qQueue.c
 */
extern	Q_STACK*	qStack();

/**
 * Structure for array-based circular-queue data structure.
 */
struct _Q_STACK {
	/* public member methods */
	size_t		(*setSize)	(Q_STACK *stack, size_t max);

	bool		(*push)		(Q_STACK *stack, const void *data, size_t size);
	bool		(*pushStr)	(Q_STACK *stack, const char *str);
	bool		(*pushInt)	(Q_STACK *stack, int num);

	void*		(*pop)		(Q_STACK *stack, size_t *size);
	char*		(*popStr)	(Q_STACK *stack);
	int		(*popInt)	(Q_STACK *stack);
	void*		(*popAt)	(Q_STACK *stack, int index, size_t *size);

	void*		(*get)		(Q_STACK *stack, size_t *size, bool newmem);
	char*		(*getStr)	(Q_STACK *stack);
	int		(*getInt)	(Q_STACK *stack);
	void*		(*getAt)	(Q_STACK *stack, int index, size_t *size, bool newmem);

	size_t		(*size)		(Q_STACK *stack);
	void		(*clear)	(Q_STACK *stack);
	bool		(*debug)	(Q_STACK *stack, FILE *out);
	void		(*free)		(Q_STACK *stack);

	/* private variables */
	Q_LIST		*list;		/*!< data container */
};

/* =========================================================================
 *
 * UTILITIES SECTION
 *
 * ========================================================================= */

/*
 * qCount.c
 */
extern	int		qCountRead(const char *filepath);
extern	bool		qCountSave(const char *filepath, int number);
extern	int		qCountUpdate(const char *filepath, int number);

/*
 * qEncode.c
 */
extern	Q_LISTTBL*	qParseQueries(Q_LISTTBL *tbl, const char *query, char equalchar, char sepchar, int *count);
extern	char*		qUrlEncode(const void *bin, size_t size);
extern	size_t		qUrlDecode(char *str);
extern	char*		qBase64Encode(const void *bin, size_t size);
extern	size_t		qBase64Decode(char *str);
extern	char*		qHexEncode(const void *bin, size_t size);
extern	size_t		qHexDecode(char *str);

/*
 * qFile.c
 */
extern	bool		qFileLock(int fd);
extern	bool		qFileUnlock(int fd);
extern	bool		qFileExist(const char *filepath);
extern	void*		qFileLoad(const char *filepath, size_t *nbytes);
extern	void*		qFileRead(FILE *fp, size_t *nbytes);
extern	ssize_t		qFileSave(const char *filepath, const void *buf, size_t size, bool append);
extern	bool		qFileMkdir(const char *dirpath, mode_t mode, bool recursive);

extern	char*		qFileGetName(const char *filepath);
extern	char*		qFileGetDir(const char *filepath);
extern	char*		qFileGetExt(const char *filepath);
extern	off_t		qFileGetSize(const char *filepath);

extern	bool		qFileCheckPath(const char *path);
extern	char*		qFileCorrectPath(char *path);
extern	char*		qFileGetAbsPath(char *buf, size_t bufsize, const char *path);

/*
 * qHash.c
 */
extern	unsigned char*	qHashMd5(const void *data, size_t nbytes);
extern	char*		qHashMd5Str(const void *data, size_t nbytes);
extern	char*		qHashMd5File(const char *filepath, size_t *nbytes);
extern	uint32_t	qHashFnv32(const void *data, size_t nbytes);
extern	uint64_t	qHashFnv64(const void *data, size_t nbytes);

/*
 * qIo.c
 */
extern	int		qIoWaitReadable(int fd, int timeoutms);
extern	int		qIoWaitWritable(int fd, int timeoutms);
extern	ssize_t		qIoRead(int fd, void *buf, size_t nbytes, int timeoutms);
extern	ssize_t		qIoWrite(int fd, const void *data, size_t nbytes, int timeoutms);
extern	off_t		qIoSend(int outfd, int infd, off_t nbytes, int timeoutms);
extern	ssize_t		qIoGets(int fd, char *buf, size_t bufsize, int timeoutms);
extern	ssize_t		qIoPuts(int fd, const char *str, int timeoutms);
extern	ssize_t		qIoPrintf(int fd, int timeoutms, const char *format, ...);

/*
 * qLibc.c
 */
extern	const char*	qLibcVersion(void);
extern	bool		qLibcThreadsafe(void);
extern	bool		qLibcLfs(void);

/*
 * qSocket.c
 */
extern	int		qSocketOpen(const char *hostname, int port, int timeoutms);
extern	bool		qSocketClose(int sockfd, int timeoutms);
extern	bool		qSocketGetAddr(struct sockaddr_in *addr, const char *hostname, int port);

/*
 * qString.c
 */
extern	char*		qStrTrim(char *str);
extern	char*		qStrTrimTail(char *str);
extern	char*		qStrUnchar(char *str, char head, char tail);
extern	char*		qStrReplace(const char *mode, char *srcstr, const char *tokstr, const char *word);
extern	char*		qStrCpy(char *dst, size_t size, const char *src);
extern	char*		qStrCpyn(char *dst, size_t size, const char *src, size_t nbytes);
extern	char*		qStrDupf(const char *format, ...);
extern	char*		qStrGets(char *buf, size_t size, char **offset);
extern	char*		qStrUpper(char *str);
extern	char*		qStrLower(char *str);
extern	char*		qStrRev(char *str);
extern	char*		qStrTok(char *str, const char *delimiters, char *retstop, int *offset);
extern	Q_LIST*		qStrTokenizer(const char *str, const char *delimiters);
extern	char*		qStrCommaNumber(int number);
extern	char*		qStrCatf(char *str, const char *format, ...);
extern	char*		qStrDupBetween(const char *str, const char *start, const char *end);
extern	char*		qStrUnique(const char *seed);
extern	bool		qStrTest(int (*testfunc)(int), const char *str);
extern	bool		qStrIsEmail(const char *email);
extern	bool		qStrIsIpv4Addr(const char *str);
extern	char*		qStrConvEncoding(const char *fromstr, const char *fromcode, const char *tocode, float mag);

/*
 * qSystem.c
 */
extern	const char*	qSysGetEnv(const char *envname, const char *nullstr);
extern	char*		qSysCmd(const char *cmd);
extern	bool		qSysGetIp(char *buf, size_t bufsize);

/*
 * qTime.c
 */
extern	char*		qTimeGetLocalStrf(char *buf, int size, time_t utctime, const char *format);
extern	char*		qTimeGetLocalStr(time_t utctime);
extern	const char*	qTimeGetLocalStaticStr(time_t utctime);
extern	char*		qTimeGetGmtStrf(char *buf, int size, time_t utctime, const char *format);
extern	char*		qTimeGetGmtStr(time_t utctime);
extern	const char*	qTimeGetGmtStaticStr(time_t utctime);
extern	time_t		qTimeParseGmtStr(const char *gmtstr);


/* =========================================================================
 *
 * IPC SECTION
 *
 * ========================================================================= */

/*
 * qSem.c
 */
extern	int		qSemInit(const char *keyfile, int keyid, int nsems, bool recreate);
extern	int		qSemGetId(const char *keyfile, int keyid);
extern	bool		qSemEnter(int semid, int semno);
extern	bool		qSemEnterNowait(int semid, int semno);
extern	bool		qSemEnterForce(int semid, int semno, int maxwaitms, bool *forceflag);
extern	bool		qSemLeave(int semid, int semno);
extern	bool		qSemCheck(int semid, int semno);
extern	bool		qSemFree(int semid);

/*
 * qShm.c
 */
extern	int		qShmInit(const char *keyfile, int keyid, size_t size, bool recreate);
extern	int		qShmGetId(const char *keyfile, int keyid);
extern	void*		qShmGet(int shmid);
extern	bool		qShmFree(int shmid);

/* =========================================================================
 *
 * EXTENSIONS SECTION
 *
 * ========================================================================= */

/* -------------------------------------------------------------------------
 * Configuration Parser.
 * qConfig.c
 * ------------------------------------------------------------------------- */
extern	Q_LISTTBL*	qConfigParseFile(Q_LISTTBL *tbl, const char *filepath, char sepchar);
extern	Q_LISTTBL*	qConfigParseStr(Q_LISTTBL *tbl, const char *str, char sepchar);

/* -------------------------------------------------------------------------
 * Q_LOG - Rotating file logger.
 * qLog.c
 * ------------------------------------------------------------------------- */

/**
 * Q_LOG types and definitions.
 */
typedef struct _Q_LOG	Q_LOG;

/**
 * Q_LOG constructor.
 */
 extern	Q_LOG*		qLog(const char *filepathfmt, mode_t mode, int rotateinterval, bool flush);

/**
 * Q_LOG details.
 */
struct _Q_LOG {
	/* public member methods */
	bool	(*write)		(Q_LOG *log, const char *str);
	bool	(*writef)		(Q_LOG *log, const char *format, ...);
	bool	(*duplicate)		(Q_LOG *log, FILE *outfp, bool flush);
	bool	(*flush)		(Q_LOG *log);
	bool	(*free)			(Q_LOG *log);

	/* private variables */
	Q_MUTEX_T	qmutex;		/*!< activated if compiled with --enable-threadsafe option */

	char	filepathfmt[PATH_MAX];	/*!< file file naming format like /somepath/daily-%Y%m%d.log */
	char	filepath[PATH_MAX];	/*!< generated system path of log file */
	FILE	*fp;			/*!< file pointer of logpath */
	mode_t	mode;			/*!< file mode */
	int	rotateinterval;		/*!< log file will be rotate in this interval seconds */
	int	nextrotate;		/*!< next rotate universal time, seconds */
	bool	logflush;		/*!< flag for immediate flushing */

	FILE	*outfp;			/*!< stream pointer for duplication */
	bool	outflush;		/*!< flag for immediate flushing for duplicated stream */
};

/* -------------------------------------------------------------------------
 * Q_HTTPCLIENT - HTTP client implementation.
 * qHttpClient.c
 * ------------------------------------------------------------------------- */

/**
 * Q_HTTPCLIENT types and definitions.
 */
typedef struct _Q_HTTPCLIENT	Q_HTTPCLIENT;

/**
 * Q_HTTPCLIENT constructor.
 */
extern	Q_HTTPCLIENT*	qHttpClient(const char *hostname, int port);

/**
 * Q_HTTPCLIENT details.
 */
struct _Q_HTTPCLIENT {
	/* public member methods */
	bool		(*setSsl)		(Q_HTTPCLIENT *client);
	void		(*setTimeout)		(Q_HTTPCLIENT *client, int timeoutms);
	void		(*setKeepalive)		(Q_HTTPCLIENT *client, bool keepalive);
	void		(*setUseragent)		(Q_HTTPCLIENT *client, const char *useragent);

	bool		(*open)			(Q_HTTPCLIENT *client);

	bool		(*head)			(Q_HTTPCLIENT *client, const char *uri, int *rescode, Q_LISTTBL *reqheaders, Q_LISTTBL *resheaders);
	bool		(*get)			(Q_HTTPCLIENT *client, const char *uri, int fd, off_t *savesize, int *rescode, Q_LISTTBL *reqheaders, Q_LISTTBL *resheaders, bool (*callback)(void *userdata, off_t recvbytes), void *userdata);
	bool		(*put)			(Q_HTTPCLIENT *client, const char *uri, int fd, off_t length, int *retcode, Q_LISTTBL *userheaders, Q_LISTTBL *resheaders, bool (*callback)(void *userdata, off_t sentbytes), void *userdata);
	void*		(*cmd)			(Q_HTTPCLIENT *client, const char *method, const char *uri, int *rescode, size_t *contentslength, Q_LISTTBL *reqheaders, Q_LISTTBL *resheaders);

	bool		(*sendRequest)		(Q_HTTPCLIENT *client, const char *method, const char *uri, Q_LISTTBL *reqheaders);
	int		(*readResponse)		(Q_HTTPCLIENT *client, Q_LISTTBL *resheaders, off_t *contentlength);

	ssize_t		(*gets)			(Q_HTTPCLIENT *client, char *buf, size_t bufsize);
	ssize_t		(*read)			(Q_HTTPCLIENT *client, void *buf, size_t nbytes);
	ssize_t		(*write)		(Q_HTTPCLIENT *client, const void *buf, size_t nbytes);
	off_t		(*recvfile)		(Q_HTTPCLIENT *client, int fd, off_t nbytes);
	off_t		(*sendfile)		(Q_HTTPCLIENT *client, int fd, off_t nbytes);

	bool		(*close)		(Q_HTTPCLIENT *client);
	void		(*free)			(Q_HTTPCLIENT *client);

	/* private variables */
	int		socket;			/*!< socket descriptor */
	void		*ssl;			/*!< will be used if SSL has been enabled at compile time */

	struct sockaddr_in addr;
	char		*hostname;
	int		port;

	int		timeoutms;		/*< wait timeout miliseconds*/
	bool		keepalive;		/*< keep-alive flag */
	char*		useragent;		/*< user-agent name */

	bool		connclose;		/*< response keep-alive flag for a last request */
};

/* -------------------------------------------------------------------------
 * Q_DB - Database interface.
 * qDatabase.c
 * ------------------------------------------------------------------------- */

/**
 * Q_DB & Q_DBRESULT types and definitions.
 */
typedef struct _Q_DB		Q_DB;
typedef struct _Q_DBRESULT	Q_DBRESULT;

/* Database Support*/
#ifdef _mysql_h
#define _Q_ENABLE_MYSQL		(1)
#endif	/* _mysql_h */

/**
 * Q_DB constructor.
 */
extern	Q_DB*		qDb(const char *dbtype, const char *addr, int port, const char *database, const char *username, const char *password, bool autocommit);

/**
 * Q_DB details.
 */
struct _Q_DB {
	/* public member methods */
	bool		(*open)			(Q_DB *db);
	bool		(*close)		(Q_DB *db);

	int		(*executeUpdate)	(Q_DB *db, const char *query);
	int		(*executeUpdatef)	(Q_DB *db, const char *format, ...);

	Q_DBRESULT*	(*executeQuery)		(Q_DB *db, const char *query);
	Q_DBRESULT*	(*executeQueryf)	(Q_DB *db, const char *format, ...);

	bool		(*beginTran)		(Q_DB *db);
	bool		(*endTran)		(Q_DB *db, bool commit);
	bool		(*commit)		(Q_DB *db);
	bool		(*rollback)		(Q_DB *db);

	bool		(*setFetchType)		(Q_DB *db, bool use);
	bool		(*getConnStatus)	(Q_DB *db);
	bool		(*ping)			(Q_DB *db);
	const char*	(*getError)		(Q_DB *db, unsigned int *errorno);
	bool		(*free)			(Q_DB *db);

	/* private variables */
	Q_MUTEX_T	qmutex;		/*!< activated if compiled with --enable-threadsafe option */

	bool connected;			/*!< if opened true, if closed false */

	struct {
		char*	dbtype;
		char*	addr;
		int	port;
		char*	username;
		char*	password;
		char*	database;
		bool	autocommit;
		bool	fetchtype;
	} info;				/*!< database connection infomation */

	/* private variables for mysql database */
#ifdef _Q_ENABLE_MYSQL
	MYSQL		*mysql;
#endif
};

/**
 * Q_DBRESULT details.
 */
struct _Q_DBRESULT {
	/* public member methods */
	const char*	(*getStr)		(Q_DBRESULT *result, const char *field);
	const char*	(*getStrAt)		(Q_DBRESULT *result, int idx);
	int		(*getInt)		(Q_DBRESULT *result, const char *field);
	int		(*getIntAt)		(Q_DBRESULT *result, int idx);
	bool		(*getNext)		(Q_DBRESULT *result);

	int     	(*getCols)		(Q_DBRESULT *result);
	int     	(*getRows)		(Q_DBRESULT *result);
	int     	(*getRow)		(Q_DBRESULT *result);

	bool		(*free)			(Q_DBRESULT *result);

	/* private variables */

	/* private variables for mysql database */
#ifdef _Q_ENABLE_MYSQL
	bool		fetchtype;
	MYSQL_RES	*rs;
	MYSQL_FIELD	*fields;
	MYSQL_ROW	row;
	int		cols;
	int		cursor;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /*_QLIBC_H */
