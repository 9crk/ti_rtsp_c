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
 * rtsp_resp.c - process rtsp response
 */

#include "rtsp_private.h"
#define MIN(a,b) (a<b)? a : b

static const char *end2 = "\n\r";
static const char *end1 = "\r\n";
static const char *end3 = "\r";
static const char *end4 = "\n";

/*
 * find_end_seperator()
 * Will search through the string pointer to by ptr, and figure out
 * what character is being used as a seperator - either \r \n, \r\n
 * or \n\r.
 *
 * Inputs:
 *    ptr - string to decode
 *    len - pointer to end seperator len
 *
 * Outputs:
 *    pointer to character to use as end seperator.  Dividing the len
 *    by 2, and adding to the return pointer will give the normal
 *    line seperator.
 */
static const char *find_seperator (const char *ptr)
{
  while (*ptr != '\0') {
    if (*ptr == '\r') {
      if (*(ptr + 1) == '\n') {
	return (end1);
      } else {
	return (end3);
      }
    } else if (*ptr == '\n') {
      if (*(ptr + 1) == '\r') {
	return (end2);
      } else {
	return (end4);
      }
    }
    ptr++;
  }
  return (NULL);
}

/************************************************************************
 * Decode rtsp header lines.
 *
 * These routines will decode the various RTSP header lines, setting the
 * correct information in the rtsp_decode_t structure.
 *
 * DEC_DUP_WARN macro will see if the field is already set, and send a warning
 * if it is.  If the cont_line field is set, it will append the next line
 *
 ************************************************************************/
#define RTSP_HEADER_FUNC(a) static void a (const char *lptr, rtsp_decode_t *dec, int cont_line)

static void dec_dup_warn (char **location,
			  const char *lptr,
			  const char *warn,
			  int cont_line)
{
  if (*location == NULL) {
    *location = strdup(lptr);
  } else if (cont_line != 0) {
    uint32_t len = strlen(*location);
    len += strlen(lptr);
    len++;
    *location = realloc(*location, len);
    strcat(*location, lptr);
  } else {
/*   printf( "2nd %s %s\n", warn, lptr);
*/
  }
}

#define DEC_DUP_WARN(a, b) dec_dup_warn(&dec->a, lptr, b, cont_line)

RTSP_HEADER_FUNC(rtsp_header_allow_public)
{
  DEC_DUP_WARN(allow_public, "allow public");
}

RTSP_HEADER_FUNC(rtsp_header_connection)
{
  if (strncasecmp(lptr, "close", strlen("close")) == 0) {
    dec->close_connection = TRUE;
  }
}

RTSP_HEADER_FUNC(rtsp_header_cookie)
{
  DEC_DUP_WARN(cookie, "cookie");
}

RTSP_HEADER_FUNC(rtsp_header_content_base)
{
  DEC_DUP_WARN(content_base, "Content-Base");
}

RTSP_HEADER_FUNC(rtsp_header_content_length)
{
  dec->content_length = (uint32_t)strtoul(lptr, NULL, 10);
}

RTSP_HEADER_FUNC(rtsp_header_content_loc)
{
  DEC_DUP_WARN(content_location, "Content-Location");
}

RTSP_HEADER_FUNC(rtsp_header_content_type)
{
  DEC_DUP_WARN(content_type, "Content-Type");
}

RTSP_HEADER_FUNC(rtsp_header_cseq)
{
  dec->cseq = (uint32_t)strtoul(lptr, NULL, 10);
}

RTSP_HEADER_FUNC(rtsp_header_location)
{
  DEC_DUP_WARN(location, "Location");
}

RTSP_HEADER_FUNC(rtsp_header_range)
{
  DEC_DUP_WARN(range, "Range");
}

RTSP_HEADER_FUNC(rtsp_header_retry_after)
{
  /* May try strptime if we need to convert*/
  DEC_DUP_WARN(retry_after, "Retry-After");
}

RTSP_HEADER_FUNC(rtsp_header_rtp)
{
  DEC_DUP_WARN(rtp_info, "RtpInfo");
}

RTSP_HEADER_FUNC(rtsp_header_session)
{
  DEC_DUP_WARN(session, "Session");
}

RTSP_HEADER_FUNC(rtsp_header_speed)
{
  DEC_DUP_WARN(speed, "Speed");
}

RTSP_HEADER_FUNC(rtsp_header_transport)
{
  DEC_DUP_WARN(transport, "Transport");
}

RTSP_HEADER_FUNC(rtsp_header_www)
{
  DEC_DUP_WARN(www_authenticate, "WWW-Authenticate");
}

RTSP_HEADER_FUNC(rtsp_header_accept)
{
  DEC_DUP_WARN(accept, "Accept");
}

RTSP_HEADER_FUNC(rtsp_header_accept_enc)
{
  DEC_DUP_WARN(accept_encoding, "Accept-Encoding");
}

RTSP_HEADER_FUNC(rtsp_header_accept_lang)
{
  DEC_DUP_WARN(accept_language, "Accept-Language");
}

RTSP_HEADER_FUNC(rtsp_header_auth)
{
  DEC_DUP_WARN(authorization, "Authorization");
}

RTSP_HEADER_FUNC(rtsp_header_bandwidth)
{
  DEC_DUP_WARN(bandwidth, "Bandwidth");
}

RTSP_HEADER_FUNC(rtsp_header_blocksize)
{
  DEC_DUP_WARN(blocksize, "blocksize");
}

RTSP_HEADER_FUNC(rtsp_header_cache_control)
{
  DEC_DUP_WARN(cache_control, "Cache-control:");
}

RTSP_HEADER_FUNC(rtsp_header_content_enc)
{
  DEC_DUP_WARN(content_encoding, "Content-Encoding:");
}

RTSP_HEADER_FUNC(rtsp_header_content_lang)
{
  DEC_DUP_WARN(content_language, "Content-Language:");
}
RTSP_HEADER_FUNC(rtsp_header_date)
{
  DEC_DUP_WARN(date, "Date:");
}

RTSP_HEADER_FUNC(rtsp_header_expires)
{
  DEC_DUP_WARN(expires, "Expires:");
}

RTSP_HEADER_FUNC(rtsp_header_from)
{
  DEC_DUP_WARN(from, "From:");
}

RTSP_HEADER_FUNC(rtsp_header_ifmod)
{
  DEC_DUP_WARN(if_modified_since, "If-Modified-Since:");
}

RTSP_HEADER_FUNC(rtsp_header_lastmod)
{
  DEC_DUP_WARN(last_modified, "Last-Modified:");
}

RTSP_HEADER_FUNC(rtsp_header_proxyauth)
{
  DEC_DUP_WARN(proxy_authenticate, "Proxy-Authenticate:");
}

RTSP_HEADER_FUNC(rtsp_header_proxyreq)
{
  DEC_DUP_WARN(proxy_require, "Proxy-Require:");
}

RTSP_HEADER_FUNC(rtsp_header_referer)
{
  DEC_DUP_WARN(referer, "Referer:");
}

RTSP_HEADER_FUNC(rtsp_header_scale)
{
  DEC_DUP_WARN(scale, "Scale:");
}

RTSP_HEADER_FUNC(rtsp_header_server)
{
  DEC_DUP_WARN(server, "Server:");
}

RTSP_HEADER_FUNC(rtsp_header_unsup)
{
  DEC_DUP_WARN(unsupported, "Unsupported:");
}

RTSP_HEADER_FUNC(rtsp_header_uagent)
{
  DEC_DUP_WARN(user_agent, "User-Agent:");
}

RTSP_HEADER_FUNC(rtsp_header_via)
{
  DEC_DUP_WARN(via, "Via:");
}

/*
 * header_types structure will provide a lookup between certain headers
 * in RTSP, and the function routines (above) that deal with actions based
 * on the response
 */
static struct {
  const char *val;
  uint32_t val_length;
  void (*parse_routine)(const char *lptr, rtsp_decode_t *decode, int cont_line);
} header_types[] =
{
#define HEAD_TYPE(a, b) { a, sizeof(a), b }
  HEAD_TYPE("Allow:", rtsp_header_allow_public),
  HEAD_TYPE("Public:", rtsp_header_allow_public),
  HEAD_TYPE("Connection:", rtsp_header_connection),
  HEAD_TYPE("Content-Base:", rtsp_header_content_base),
  HEAD_TYPE("Content-Length:", rtsp_header_content_length),
  HEAD_TYPE("Content-Location:", rtsp_header_content_loc),
  HEAD_TYPE("Content-Type:", rtsp_header_content_type),
  HEAD_TYPE("CSeq:", rtsp_header_cseq),
  HEAD_TYPE("Location:", rtsp_header_location),
  HEAD_TYPE("Range:", rtsp_header_range),
  HEAD_TYPE("Retry-After:", rtsp_header_retry_after),
  HEAD_TYPE("RTP-Info:", rtsp_header_rtp),
  HEAD_TYPE("Session:", rtsp_header_session),
  HEAD_TYPE("Set-Cookie:", rtsp_header_cookie),
  HEAD_TYPE("Speed:", rtsp_header_speed),
  HEAD_TYPE("Transport:", rtsp_header_transport),
  HEAD_TYPE("WWW-Authenticate:", rtsp_header_www),
  /* None of these are needed for client, but are included for completion*/
  HEAD_TYPE("Accept:", rtsp_header_accept),
  HEAD_TYPE("Accept-Encoding:", rtsp_header_accept_enc),
  HEAD_TYPE("Accept-Language:", rtsp_header_accept_lang),
  HEAD_TYPE("Authorization:", rtsp_header_auth),
  HEAD_TYPE("Bandwidth:", rtsp_header_bandwidth),
  HEAD_TYPE("Blocksize:", rtsp_header_blocksize),
  HEAD_TYPE("Cache-Control:", rtsp_header_cache_control),
  HEAD_TYPE("Content-Encoding:", rtsp_header_content_enc),
  HEAD_TYPE("Content-Language:", rtsp_header_content_lang),
  HEAD_TYPE("Date:", rtsp_header_date),
  HEAD_TYPE("Expires:", rtsp_header_expires),
  HEAD_TYPE("From:", rtsp_header_from),
  HEAD_TYPE("If-Modified-Since:", rtsp_header_ifmod),
  HEAD_TYPE("Last-Modified:", rtsp_header_lastmod),
  HEAD_TYPE("Proxy-Authenticate:", rtsp_header_proxyauth),
  HEAD_TYPE("Proxy-Require:", rtsp_header_proxyreq),
  HEAD_TYPE("Referer:", rtsp_header_referer),
  HEAD_TYPE("Scale:", rtsp_header_scale),
  HEAD_TYPE("Server:", rtsp_header_server),
  HEAD_TYPE("Unsupported:", rtsp_header_unsup),
  HEAD_TYPE("User-Agent:", rtsp_header_uagent),
  HEAD_TYPE("Via:", rtsp_header_via),
  { NULL, 0, NULL },
};

/* We don't care about the following: Cache-control, date, expires,*/
/* Last-modified, Server, Unsupported, Via*/

/*
 * rtsp_decode_header - header line pointed to by lptr.  will be \0 terminated
 * Decode using above table
 */
static void rtsp_decode_header (const char *lptr,
				rtsp_client_t *info,
				int *last_number)
{
  int ix;
  const char *after;

  ix = 0;
  /*
   * go through above array, looking for a complete match
   */
  while (header_types[ix].val != NULL) {
    if (strncasecmp(lptr,
		    header_types[ix].val,
		    header_types[ix].val_length - 1) == 0) {
      after = lptr + header_types[ix].val_length;

      ADV_SPACE(after);
      /*
       * Call the correct parsing routine
       */
      (header_types[ix].parse_routine)(after, info->decode_response, 0);
      *last_number = ix;
      return;
    }
    ix++;
  }

  if (*last_number >= 0 && isspace(*lptr)) {
    /*asdf*/
    ADV_SPACE(lptr);
    (header_types[*last_number].parse_routine)(lptr, info->decode_response, 1);
  } else
   printf( "Not processing response header: %s\n", lptr);
}
static int rtsp_read_into_buffer (rtsp_client_t *cptr,
				  uint32_t buffer_offset,
				  int wait)
{
  int ret;

  ret = rtsp_receive_socket(cptr,
			    cptr->m_resp_buffer + buffer_offset,
			    RECV_BUFF_DEFAULT_LEN - buffer_offset,
			    cptr->recv_timeout,
			    wait);

  if (ret <= 0) return (ret);

  cptr->m_buffer_len = buffer_offset + ret;
  cptr->m_resp_buffer[cptr->m_buffer_len] = '\0';

  return ret;
}

int rtsp_bytes_in_buffer (rtsp_client_t *cptr)
{
  return (cptr->m_buffer_len - cptr->m_offset_on);
}

int rtsp_recv (rtsp_client_t *cptr,
	       char *buffer,
	       uint32_t len)
{
  uint32_t mlen;
  int copied, result;

  copied = 0;
  if (cptr->m_offset_on < cptr->m_buffer_len) {
    mlen =  MIN(len, cptr->m_buffer_len - cptr->m_offset_on);
    memmove(buffer,
	    &cptr->m_resp_buffer[cptr->m_offset_on],
	    mlen);
    cptr->m_offset_on += mlen;
    len -= mlen;
    copied += mlen;
  }
  if (len > 0) {
    result = rtsp_receive_socket(cptr,
				 buffer + copied,
				 len,
				 0,
				 0);
    if (result >= 0) {
      copied += result;
    }
  }
  return copied;
}


static int rtsp_save_and_read (rtsp_client_t *cptr)
{
  int last_on;
  int ret;
  cptr->m_buffer_len -= cptr->m_offset_on;
  if (cptr->m_buffer_len >= RECV_BUFF_DEFAULT_LEN) {
    return 0;
  } else if (cptr->m_buffer_len != 0) {
    memmove(cptr->m_resp_buffer,
	    &cptr->m_resp_buffer[cptr->m_offset_on],
	    cptr->m_buffer_len);
    last_on = cptr->m_buffer_len;
  } else {
    last_on = 0;
  }

  cptr->m_offset_on = 0;
  ret = rtsp_read_into_buffer(cptr, cptr->m_buffer_len, 0);
  if (ret > 0) {
    last_on += ret;
  }
  return (last_on);
}

static const char *rtsp_get_next_line (rtsp_client_t *cptr,
				       const char *seperator)
{
  int ret;
  int last_on;
  char *sep;

  /*
   * If we don't have any data, try to read a buffer full
   */
  if (cptr->m_buffer_len == 0) {
    cptr->m_offset_on = 0;
    ret = rtsp_read_into_buffer(cptr, 0, 1);
    if (ret <= 0) {
      return (NULL);
    }
  }

  /*
   * Look for CR/LF in the buffer.  If we find it, NULL terminate at
   * the CR, then set the buffer values to look past the crlf
   */
  sep = strstr(&cptr->m_resp_buffer[cptr->m_offset_on],
	       seperator);
  if (sep != NULL) {
    const char *retval = &cptr->m_resp_buffer[cptr->m_offset_on];
    cptr->m_offset_on = sep - cptr->m_resp_buffer;
    cptr->m_offset_on += strlen(seperator);
    *sep = '\0';
    return (retval);
  }

  if (cptr->m_offset_on == 0) {
    return (NULL);
  }

  /*
   * We don't have a line.  So, move the data down in the buffer, then
   * read into the rest of the buffer
   */
  last_on = rtsp_save_and_read(cptr);
  if (last_on < 0) return (NULL);
  /*
   * Continue searching through the buffer.  If we get this far, we've
   * received like 2K or 4K without a CRLF - most likely a bad response
   */
  sep = strstr(&cptr->m_resp_buffer[last_on],
	       seperator);
  if (sep != NULL) {
    const char *retval = &cptr->m_resp_buffer[cptr->m_offset_on];
    cptr->m_offset_on = sep - cptr->m_resp_buffer;
    cptr->m_offset_on += strlen(seperator);
    *sep = '\0';
    return (retval);
  }
  return (NULL);
}
/*
 * rtsp_parse_response - parse out response headers lines.  Figure out
 * where seperators are, then use them to determine where the body is
 */
static int rtsp_parse_response (rtsp_client_t *info)
{
  const char *seperator;
  const char *lptr, *p;
  rtsp_decode_t *decode;
  int ret, done;
  int last_header = -1;
  uint32_t len;

  decode = info->decode_response;

  /* Figure out what seperator is being used throughout the response*/
  ret = rtsp_save_and_read(info);
  if (ret < 0) {
    return (RTSP_RESPONSE_RECV_ERROR);
  }

  rtsp_print("rtsp response:\n");
  rtsp_print("%s\n", info->m_resp_buffer);
  seperator = find_seperator(info->m_resp_buffer);
  if (seperator == NULL) {
   printf( "Could not find seperator in header\n");
    return RTSP_RESPONSE_MALFORM_HEADER;
  }

  /*rtsp_debug(LOG_DEBUG, "Response buffer:\n%s", info->m_resp_buffer);*/

  do {
    lptr = rtsp_get_next_line(info, seperator);
    if (lptr == NULL) {
     printf( "Couldn't get next line\n");
      return (RTSP_RESPONSE_MALFORM_HEADER);
    }
  } while (*lptr == '\0');

/* printf( "%s\n",lptr);
*/
  if (strncasecmp(lptr, "RTSP/1.0", strlen("RTSP/1.0")) != 0) {
   printf( "RTSP/1.0 not found\n");
    return RTSP_RESPONSE_MALFORM_HEADER;
  }
  p = lptr + strlen("RTSP/1.0\n");
  ADV_SPACE(p);
  memcpy(decode->retcode, p, 3);
  decode->retcode[3] = '\0';

  switch (*p) {
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    break;
  default:
   printf( "Bad error code %s\n", p);
    return RTSP_RESPONSE_MALFORM_HEADER;
  }
  p += 3;
  ADV_SPACE(p);
  if (p != '\0') {
    decode->retresp = strdup(p);
  }
/* printf( "Decoded status - code %s %s\n",
	     decode->retcode, decode->retresp);
*/
  done = 0;
  do {
    lptr = rtsp_get_next_line(info, seperator);

    if (lptr == NULL) {
      return (RTSP_RESPONSE_MALFORM_HEADER);
    }
    if (*lptr == '\0') {
      done = 1;
    } else {
/*     printf( "%s\n",lptr);
*/
      rtsp_decode_header(lptr, info, &last_header);
    }
  } while (done == 0);

  if (decode->content_length != 0) {
/*   printf( "reading content length %d\n", decode->content_length);
*/
    decode->body = malloc(decode->content_length + 1);
    decode->body[decode->content_length] = '\0';
    len = info->m_buffer_len - info->m_offset_on;
/*   printf( "Copied %d bytes from buffer\n", len);
*/
    if (len < decode->content_length) {
      /* not enough in memory - need to read*/
      memcpy(decode->body,
	     &info->m_resp_buffer[info->m_offset_on],
	     len);
      while (len < decode->content_length) {
	int left;
	ret = rtsp_read_into_buffer(info, 0, 1);

	if (ret <= 0) {
	 printf( "Returned from rtsp_read_into_buffer - error %d\n",
		     ret);
	  return (-1);
	}
	left = decode->content_length - len;
	if (left < info->m_buffer_len) {
	  /* we have more data in the buffer then we need*/
	  memcpy(decode->body + len,
		 info->m_resp_buffer,
		 left);
	  len += left;
	  info->m_offset_on = left;
	} else {
	  /* exact or less - we're cool*/
	  memcpy(decode->body + len,
		 info->m_resp_buffer,
		 info->m_buffer_len);
	  len += info->m_buffer_len;
	  info->m_offset_on = info->m_buffer_len; /* get ready for next...*/
	}
      }
    } else {
      /* Plenty in memory - copy it, continue to end...*/
      memcpy(decode->body,
	     &info->m_resp_buffer[info->m_offset_on],
	     decode->content_length);
      info->m_offset_on += decode->content_length;
    }
  } else if (decode->close_connection) {
    /* No termination - just deal with what we've got...*/
    len = info->m_buffer_len - info->m_offset_on;
    decode->body = (char *)malloc(len + 1);
    memcpy(decode->body,
	   &info->m_resp_buffer[info->m_offset_on],
	   len);
    decode->body[len] = '\0';
  }
  /*if (decode->body != NULL)*/
  /*{
        int tocopy2=0;
      while( len > 0)
        {
            U32 tmp=0;
            tmp = NG_MIN( len, 250);
            memcpy(decode->body+tocopy2,
    	   info->m_resp_buffer + info->m_offset_on +tocopy2,
    	   tmp);
            decode->body[tocopy2+tmp] = '\0';
            printf("%s",decode->body+tocopy2);
            len-=tmp;
            tocopy2+=tmp;
        }
    }*/
  return (0);
}

/*
 * rtsp_get_response - wait for a complete response.  Interesting that
 * it could be in multiple packets, so deal with that.
 */
int rtsp_get_response (rtsp_client_t *info)
{
  int ret;
  rtsp_decode_t *decode;
  int response_okay;

  while (1) {
    /* In case we didn't get a response that we wanted.*/
    if (info->decode_response != NULL) {
      clear_decode_response(info->decode_response);
      decode = info->decode_response;
    } else {
      decode = info->decode_response = malloc(sizeof(rtsp_decode_t));
      if (decode == NULL) {
	printf( "Couldn't create decode response\n");
	return (RTSP_RESPONSE_RECV_ERROR);
      }
    }

    memset(decode, 0, sizeof(rtsp_decode_t));

    do {
      /* Parse response.*/
      ret = rtsp_parse_response(info);
/*     printf( "parsed response - len %d offset %d\n",
		 info->m_buffer_len, info->m_offset_on);
*/
      if (ret != 0) {
/*	if (info->thread == NULL)*/
	  rtsp_close_socket(info);
	printf( "return code %d from rtsp_parse_response\n", ret);
	return (RTSP_RESPONSE_RECV_ERROR);
      }

      if (decode->close_connection) {
/*	printf("Response requested connection close\n");
*/
	rtsp_close_socket(info);
      }

      response_okay = TRUE;
      /* Okay - we have a good response. - check the cseq, and return*/
      /* the correct error code.*/
      if (info->next_cseq == decode->cseq) {
	info->next_cseq++;
	if ((decode->retcode[0] == '4') ||
	    (decode->retcode[0] == '5')) {
	  return (RTSP_RESPONSE_BAD);
	}
	if (decode->cookie != NULL) {
	  /* move cookie to rtsp_client*/
	  info->cookie = strdup(decode->cookie);
	}
	if (decode->retcode[0] == '2') {
	  return (RTSP_RESPONSE_GOOD);
	}
	if (decode->retcode[0] == '3') {
	  if (decode->location == NULL) {
	    return (RTSP_RESPONSE_BAD);
	  }
	  if (rtsp_setup_redirect(info) != 0) {
	    return (RTSP_RESPONSE_BAD);
	  }
	  return (RTSP_RESPONSE_REDIRECT);
	}
      }
#if 0
      if (info->m_buffer_len != 0) {
	/* We have more in the buffer - see if it's what*/
	/* we need*/
	/* clear out last response*/
	clear_decode_response(decode);
	memset(decode, 0, sizeof(rtsp_decode_t));
      }
#endif
    } while (info->m_buffer_len != 0);
  }
  return (RTSP_RESPONSE_RECV_ERROR);
}

/* end file rtsp_resp.c */
