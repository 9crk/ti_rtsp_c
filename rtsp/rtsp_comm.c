/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *              Bill May        wmay@cisco.com
 */
/*
 * rtsp_comm.c - contains communication routines.
 */
#include "stddefs.h"
#if defined ST_OSLINUX
#include "compat.h"
#endif
#include "rtsp_private.h"

#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
 
/*
 * rtsp_create_socket()
 * creates and connects socket to server.  Requires rtsp_info_t fields
 * port, server_addr, server_name be set.
 * returns 0 for success, -1 for failure
 */
static int rtsp_get_server_address (rtsp_client_t *info)
{
#ifdef HAVE_IPv6
  struct addrinfo hints;
  char port[32];
  int error;

  snprintf(port, sizeof(port), "%d", info->port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  error = getaddrinfo(info->server_name, port, &hints, &info->addr_info);
  if (error) {
   printf( "Can't get server address info %s - error %d\n",
	       info->server_name, error);
    return error;
  }
#else
  int error;
  struct hostent *host; 
  if (inet_aton(info->server_name, &info->server_addr) != 0) return 0; 

  host = gethostbyname(info->server_name);
  if (host == NULL) {
    printf( "Can't get server host name %s\n", info->server_name);
    return (error);
  }
  memcpy(&info->server_addr, host->h_addr,4);	
  //info->server_addr = *(struct in_addr *)host->h_addr;
#endif
  return 0;
}

int rtsp_create_socket (rtsp_client_t *info)
{
#ifndef HAVE_IPv6
  struct sockaddr_in sockaddr;
#endif
  int result;

  /* Do we have a socket already - if so, go ahead*/

  if (info->server_socket != -1) {
    return (0);
  }

  if (info->server_name == NULL) {
   printf( "No server name in create socket\n");
    return (-1);
  }
  result = rtsp_get_server_address(info);
  if (result != 0) return -1;

#ifndef _WIN32
#ifdef HAVE_IPv6
  info->server_socket = socket(info->addr_info->ai_family,
			       info->addr_info->ai_socktype,
			       info->addr_info->ai_protocol);
#else
  info->server_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif
#else
  info->server_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
#endif

  if (info->server_socket == -1) {
   printf( "Couldn't create socket\n");
    return (-1);
  }


#ifndef HAVE_IPv6
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(info->port);
  sockaddr.sin_addr = info->server_addr;
#endif

#ifndef _WIN32
  result = connect(info->server_socket,
#ifndef HAVE_IPv6
		   (struct sockaddr *)&sockaddr,
		   sizeof(sockaddr)
#else
		   info->addr_info->ai_addr,
		   info->addr_info->ai_addrlen
#endif
		   );
  if (result < 0)
#else
  result = WSAConnect(info->server_socket,
	                  (struct sockaddr *)&sockaddr,
					  sizeof(sockaddr),
					  NULL,
					  NULL,
					  NULL,
					  NULL);
  if (result != 0)
#endif

  {
   printf( "Couldn't connect socket - error %s\n",
	       strerror(result)
	       );
    return (-1);
  }

/*  if (info->thread != NULL) {
#ifndef _WIN32
    result = fcntl(info->server_socket, F_GETFL);
    result = fcntl(info->server_socket, F_SETFL, result | O_NONBLOCK);
    if (result < 0) {
     printf( "Couldn't create nonblocking %s",
		 strerror(errno)
		 );
    }
#else
	rtsp_thread_set_nonblocking(info);
#endif

  }*/

  return (0);
}

/*
 * rtsp_send()
 * Sends a buffer over connected socket.  If socket isn't connected,
 * tries that first.
 * Buffer must be formatted to RTSP spec.
 * Inputs:
 *   info - pointer to rtsp_client_t for client session
 *   buff - pointer to buffer
 *   len  - length of buffer
 * Outputs:
 *   0 - success, -1 failure
 */
int rtsp_send2 (rtsp_client_t *info, const char *buff, uint32_t len)
{
  int ret;

  if (info->server_socket == -1) {
    if (rtsp_create_socket(info) != 0)
      return (-1);
  }
  ret = send(info->server_socket, buff, len, 0);
  return (ret);
}

/*
 * rtsp_receive()
 * Receives a response from server with a timeout.  If recv returns a
 * full buffer, and the last character is not \r or \n, will make a
 * bigger buffer and try to receive.
 *
 * Will set fields in rtsp_client_t.  Relevent fields are:
 *   recv_buff - pointer to receive buffer (malloc'ed so we can always add
 *           \0 at end).
 *   recv_buff_len - max size of receive buffer.
 *   recv_buff_used - number of bytes received.
 *   recv_buff_parsed - used by above routine in case we got more than
 *      1 response at a time.
 */
int rtsp_receive_socket (rtsp_client_t *info, char *buffer, uint32_t len,
			 uint32_t msec_timeout, int wait)
{

  int ret;
#ifdef HAVE_POLL
  struct pollfd pollit;
#else
  fd_set read_set;
  struct timeval timeout;
#endif

  if (msec_timeout != 0 && wait != 0) {
#ifdef HAVE_POLL
    pollit.fd = info->server_socket;
    pollit.events = POLLIN | POLLPRI;
    pollit.revents = 0;

    ret = poll(&pollit, 1, msec_timeout);
#else
    FD_ZERO(&read_set);
    FD_SET(info->server_socket, &read_set);
    timeout.tv_sec = (long)msec_timeout / 1000;
    timeout.tv_usec = ((long)msec_timeout % 1000) * 1000;
    ret = selectsocket(info->server_socket + 1, &read_set, NULL, NULL, &timeout);
#endif

    if (ret <= 0) {
     printf( "Response timed out %d %d\n", msec_timeout, ret);
      if (ret == -1) {
	printf( "Error is %s\n",
		   strerror(ret));
      }
      return (-1);
    }
  }
  /*rtsp_debug(LOG_DEBUG, "Calling recv");*/
#ifndef _WIN32
  ret = recv(info->server_socket, buffer, len, 0);
#else
  {
  WSABUF foo;
  int val;
  DWORD flag = 0;
  DWORD bytes;
  foo.buf = buffer;
  foo.len = len;
  val = WSARecv(info->server_socket, &foo, 1, &bytes, &flag, NULL, NULL);
  if (val != 0) {
	  /*rtsp_debug(LOG_DEBUG, "error is %d", WSAGetLastError());*/
	  return -1;
  }
  ret = bytes;
  /*rtsp_debug(LOG_DEBUG, "recvd %d bytes", ret);*/
  }
#endif
  return (ret);
}

/*
 * rtsp_close_socket
 * closes the socket.  Duh.
 */
void rtsp_close_socket (rtsp_client_t *info)
{
  if (info->server_socket != -1)
    closesocket(info->server_socket);
  info->server_socket = -1;
#ifdef HAVE_ST_ADDRINFO
  CHECK_AND_FREE(info->addr_info);
#endif
}
