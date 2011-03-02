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
 * @file qSocket.c Socket dandling APIs.
 */

#ifndef DISABLE_SOCKET
#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "qlibc.h"
#include "qInternal.h"

/**
 * Create a TCP socket for the remote host and port.
 *
 * @param	hostname	remote hostname
 * @param	port		remote port
 * @param	timeoutms	wait timeout milliseconds. if set to negative value, wait indefinitely.
 *
 * @return	the new socket descriptor, or -1 in case of invalid hostname, -2 in case of socket creation failure, -3 in case of connection failure.
 */
int qSocketOpen(const char *hostname, int port, int timeoutms) {
	/* host conversion */
	struct sockaddr_in addr;
	if (qSocketGetAddr(&addr, hostname, port) == false) return -1; /* invalid hostname */

	/* create new socket */
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -2; /* sockfd creation fail */

	/* set to non-block socket*/
	int flags = fcntl(sockfd, F_GETFL, 0);
	if(timeoutms >= 0) fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	/* try to connect */
	int status = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
	if( status < 0 && (errno != EINPROGRESS || qIoWaitWritable(sockfd, timeoutms) <= 0) ) {
		close(sockfd);
		return -3; /* connection failed */
	}

	/* restore to block socket */
	if(timeoutms >= 0) fcntl(sockfd, F_SETFL, flags);

	return sockfd;
}

/**
 * Close socket.
 *
 * @param	sockfd		socket descriptor
 * @param	timeoutms	if timeoutms >= 0, shut down write connection first then wait and throw out input stream data. set to -1 to close socket immediately.
 *
 * @return	true on success, or false if an error occurred.
 */
bool qSocketClose(int sockfd, int timeoutms) {
	// close connection
	if(timeoutms >= 0 && shutdown(sockfd, SHUT_WR) == 0) {
		char buf[1024];
		while(true) {
			ssize_t read = qIoRead(sockfd, buf, sizeof(buf), timeoutms);
			if(read <= 0) break;
			DEBUG("Throw %zu bytes from dummy input stream.", read);
		}
	}

	if(close(sockfd) == 0) return true;
	return false;
}

/**
 * Convert hostname to sockaddr_in structure.
 *
 * @param	addr		sockaddr_in structure pointer
 * @param	hostname	IP string address or hostname
 * @param	port		port number
 *
 * @return	true if successful, otherwise returns false.
 */
bool qSocketGetAddr(struct sockaddr_in *addr, const char *hostname, int port) {
	/* here we assume that the hostname argument contains ip address */
	memset((void*)addr, 0, sizeof(struct sockaddr_in));
	if (!inet_aton(hostname, &addr->sin_addr)) { /* fail then try another way */
		struct hostent *hp;
		if ((hp = gethostbyname (hostname)) == 0) return false;
		memcpy (&addr->sin_addr, hp->h_addr, sizeof(struct in_addr));
	}
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);

	return true;
}

#endif /* _WIN32 */
#endif /* DISABLE_SOCKET */
