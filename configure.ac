## qLibc - http://www.qdecoder.org
##
## Copyright 2010 qDecoder Project. All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##
## 1. Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in the
##    documentation and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY QDECODER PROJECT ``AS IS'' AND ANY EXPRESS
## OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED. IN NO EVENT SHALL THE QDECODER PROJECT BE LIABLE FOR ANY
## DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
## ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
## THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## $Id$

AC_COPYRIGHT([
=============================================================================
qLibc - http://www.qdecoder.org

Copyright 2010 qDecoder Project. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY QDECODER PROJECT ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE QDECODER PROJECT BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=============================================================================
])

## Internal functions
AC_DEFUN([Q_ARG_ENABLE], [
	AC_ARG_ENABLE([$1],[AS_HELP_STRING([--enable-$1], [$2])],,[enableval=no])
	if test "$enableval" = yes; then
		AC_MSG_NOTICE(['$1' feature is enabled])
		CPPFLAGS="$CPPFLAGS $3"
	fi
])

AC_DEFUN([Q_ARG_DISABLE], [
	AC_ARG_ENABLE([$1],[AS_HELP_STRING([--disable-$1], [$2])],,[enableval=yes])
	if test "$enableval" = no; then
		AC_MSG_NOTICE(['$1' feature is disabled])
		CPPFLAGS="$CPPFLAGS $3"
	fi
])

## Initialize
AC_INIT([qLibc], [1 RELEASE], [http://www.qdecoder.org/])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])

## Set path
PATH="$PATH:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
CPPFLAGS="$CPPFLAGS -I./ -I./internal -I/usr/include -I/usr/local/include"
CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
LDFLAGS="$LDFLAGS -L/usr/lib -L/usr/local/lib"

## Set autoconf setting
#AC_CANONICAL_TARGET
AC_PREFIX_DEFAULT([/usr/local])
#AC_PRESERVE_HELP_ORDER

## Checks for programs.
AC_PROG_CC
if test $ac_compiler_gnu = yes; then
	CFLAGS="-Wall -fPIC"
#else
#	AC_MSG_FAILURE([GCC is required.])
fi

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_PATH_PROG(AR, ar)
AC_PATH_PROG(CHMOD, chmod)
AC_PATH_PROG(LD, ld)
AC_PATH_PROG(RM, rm)

## Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_DIRENT
#AC_CHECK_HEADER([sys/sendfile.h])

## Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_OFF_T

## Checks for libraries.

## Checks for library functions.
#AC_CHECK_FUNCS([socket sendfile])

##
## --enable section
##

Q_ARG_ENABLE([threadsafe], [Enable thread-safe feature. When it's enabled, user applications are needed to be linked with the pthread library. (ex: -lpthread)], [-DENABLE_THREADSAFE])
if test "$enableval" = yes; then
	AC_CHECK_LIB([pthread], [main], [], AC_MSG_ERROR([Can't find pthread library.]))
fi

Q_ARG_ENABLE([lfs], [Enable large file supports.], [-DENABLE_LFS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64])
Q_ARG_DISABLE([ipc], [Disable all IPC APIs in src/ipc directory.], [-DDISABLE_IPC])
Q_ARG_DISABLE([extensions], [Disable all extension APIs in src/extensions directory.], [-DDISABLE_EXTENSIONS])

Q_ARG_ENABLE([debug], [Enable debugging output. This will print out internal debugging messages to stdout.], [-DBUILD_DEBUG])
if test "$enableval" = yes; then
	CFLAGS="$CFLAGS -g -O2"
else
	CFLAGS="$CFLAGS -g -O2"
fi

##
## --with section
##

AC_ARG_WITH([openssl],[AS_HELP_STRING([--with-openssl], [This will enable HTTPS support in Q_HTTPCLIENT extension API. When it's enabled, user applications are needed to be linked with the openssl library. (ex: -lssl)])],[withval=yes],[withval=no])
if test "$withval" = yes; then
	if test "$with_openssl" = yes; then
		with_openssl="/usr/include"
	fi

	AC_CHECK_FILE([$with_openssl/openssl/ssl.h],[withval=yes],[withval=no])
	if test "$withval" = yes; then
		AC_MSG_NOTICE([HTTPS support in Q_HTTPCLIENT API is enabled])
		CPPFLAGS="$CPPFLAGS -DENABLE_OPENSSL -I$with_openssl"
	else
		AC_MSG_FAILURE([can't find '$with_openssl/openssl/ssl.h' header. Use --with-openssl=/PATH/ to specify the directory PATH where the 'opendir' directory is located.])
	fi
fi

AC_ARG_WITH([mysql],[AS_HELP_STRING([--with-mysql], [This will enable MySQL database support in Q_DB extension API. When it's enabled, user applications are needed to be linked with the mysql library. (ex: -lmysql)])],[withval=yes],[withval=no])
if test "$withval" = yes; then
	if test "$with_mysql" = yes; then
		with_mysql="/usr/include/mysql"
	fi

	AC_CHECK_FILE([$with_mysql/mysql.h],[withval=yes],[withval=no])
	if test "$withval" = yes; then
		AC_MSG_NOTICE([MySQL support in Q_DB API is enabled])
		CPPFLAGS="$CPPFLAGS -DENABLE_MYSQL -I$with_mysql"
	else
		AC_MSG_FAILURE([can't find 'mysql.h' header. Use --with-mysql=/PATH/ to specify the directory PATH where the 'mysql.h' is located.])
	fi
fi

AC_MSG_NOTICE([CFLAGS $CFLAGS])
AC_MSG_NOTICE([CPPFLAGS $CPPFLAGS])
#AC_MSG_NOTICE([LIBS $LIBS])

## Create Makefile
AC_OUTPUT
