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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
#include "stddefs.h"
#if (defined ST_7109 && defined ST_OSLINUX)
#include "compat.h"
#endif
#include "rtsp_private.h"

/*
 * free_rtsp_client()
 * frees all memory associated with rtsp client information
 */
void free_rtsp_client (rtsp_client_t *rptr)
{
  /*if (rptr->thread != NULL) {
    rtsp_close_thread(rptr);
  } else*/ {
    rtsp_close_socket(rptr);
#ifdef _WINDOWS
    WSACleanup();
#endif
  }

  CHECK_AND_FREE(rptr->orig_url);
  CHECK_AND_FREE(rptr->url);
  CHECK_AND_FREE(rptr->server_name);
  CHECK_AND_FREE(rptr->cookie);
  free_decode_response(rptr->decode_response);
  rptr->decode_response = NULL;
  free(rptr);
}


rtsp_client_t *rtsp_create_client_common (const char *url, int *perr)
{
  int err;
  rtsp_client_t *info;

  info = malloc(sizeof(rtsp_client_t));
  if (info == NULL) {
    *perr = ST_ERROR_NO_MEMORY;
    return (NULL);
  }
  memset(info, 0, sizeof(rtsp_client_t));
  info->url = NULL;
  info->orig_url = NULL;
  info->server_name = NULL;
  info->cookie = NULL;
  info->recv_timeout = 2 * 1000;  /* default timeout is 2 seconds.*/
  info->server_socket = -1;
  info->next_cseq = 1;
  info->session = NULL;
  info->m_offset_on = 0;
  info->m_buffer_len = 0;
  info->m_resp_buffer[RECV_BUFF_DEFAULT_LEN] = '\0';
/*  info->thread = NULL;*/
  
  err = rtsp_dissect_url(info, url);
  if (err != 0) {
    printf("Couldn't decode url %d\n", err);
    *perr = err;
    free_rtsp_client(info);
    return (NULL);
  }
  return (info);
}


rtsp_client_t *rtsp_create_client (const char *url, int *err)
{
  rtsp_client_t *info;

#ifdef _WINDOWS
  WORD wVersionRequested;
  WSADATA wsaData;
  int ret;
 
  wVersionRequested = MAKEWORD( 2, 0 );
 
  ret = WSAStartup( wVersionRequested, &wsaData );
  if ( ret != 0 ) {
    /* Tell the user that we couldn't find a usable */
    /* WinSock DLL.*/
    *err = ret;
    return (NULL);
  }
#endif
  info = rtsp_create_client_common(url, err);
  if (info == NULL) return (NULL);
  *err = rtsp_create_socket(info);
  if (*err != 0) {
    printf("Couldn't create socket %d\n", err);
    free_rtsp_client(info);
    return (NULL);
  }
  return (info);
}



int rtsp_send_and_get (rtsp_client_t *info,
		       char *buffer,
		       uint32_t buflen)
{
  int ret;
  rtsp_print("rtsp send:\n");
  rtsp_print("%s\n", buffer);
  /*if (info->thread == NULL)*/ {
    ret = rtsp_send2(info, buffer, buflen);
    if (ret < 0) {
      return (RTSP_RESPONSE_RECV_ERROR);
    }

    ret = rtsp_get_response(info);
  } /*else {
   rtsp_wrap_send_and_get_t msg;
    int ret_msg;
    msg.msg = RTSP_MSG_SEND_AND_GET;
    msg.body.buffer = buffer;
    msg.body.buflen = buflen;
    ret = rtsp_thread_ipc_send_wait(info,
				    (unsigned char *)&msg,
				    sizeof(msg),
				    &ret_msg);
    if (ret != sizeof(ret_msg)) {
      return (RTSP_RESPONSE_RECV_ERROR);
    }
    ret = ret_msg;
  }*/
  return ret;
}
