/* Includes --------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

/* RTSP head files */
#include "rtsp_private.h"
#include "rtsp.h"

#define PAUSE           0x1
#define PLAY            0x2
#define SETUP           0x3

/*--- Global variable---*/

static rtsp_client_t *rtsp_client = NULL;

static  const char transport_str_ovc[] =  "MP2T/H2221/UDP;unicast;client_port=%d";

static  const char transport_str[] =  "RTP/UDP;unicast;client_port=%d-%d";

static void parse_transport_str(char * str);

typedef struct {
  U32 rtp_server_port;
  U16 rtp_client_port;
  U32 rtp_ttl;
  U32 rtp_ip;
  U32 state;
}player_t; /* maybe can be used in the future added by D.L */
static player_t *player = NULL;

#define REACH_TAG     0
#define REACH_SEQ     1
#define REACH_TIME    2
#define REACH_PORT    3
#define REACH_CLPORT  4
#define REACH_SRVPORT 5
#define REACH_TTL     6
#define REACH_DEST    7
#define REACH_SRC     8
#define REACH_SSRC    9

static void do_relative_url_to_absolute (char **control_string,
				  const char *base_url,
				  int dontfree)
{
  char *str, *cpystr;
  uint32_t cblen, malloclen;

  malloclen = cblen = strlen(base_url);

  if (base_url[cblen - 1] != '/') malloclen++;
  /*
   * If the control string is just a *, use the base url only
   */
  cpystr = *control_string;
  if (strcmp(cpystr, "*") != 0)
  {
    if (*cpystr == '/') cpystr++;

    /* duh - add 1 for \0...*/
    str = (char *)malloc(strlen(cpystr) + malloclen + 1);
    if (str == NULL)
      return;
    strcpy(str, base_url);
    if (base_url[cblen - 1] != '/')
    {
      strcat(str, "/");
    }
    if (*cpystr == '/') cpystr++;
    strcat(str, cpystr);
  }
  else
  {
    str = strdup(base_url);
  }
  if (dontfree == 0)
    free(*control_string);
  *control_string = str;
}

/*
 * convert_relative_urls_to_absolute - for every url inside the session
 * description, convert relative to absolute.
 */
static void convert_relative_urls_to_absolute (session_desc_t *sdp,
					const char *base_url)
{
  media_desc_t *media;

  if (base_url == NULL)
    return;

  if ((sdp->control_string != NULL) &&
      (strncmp(sdp->control_string, "rtsp://", strlen("rtsp://"))) != 0)
  {
    do_relative_url_to_absolute(&sdp->control_string, base_url, 0);
  }

  for (media = sdp->media; media != NULL; media = media->next)
  {
    if ((media->control_string != NULL) &&
	(strncmp(media->control_string, "rtsp://", strlen("rtsp://")) != 0))
    {
      do_relative_url_to_absolute(&media->control_string, base_url, 0);
    }
  }
}

static void parse_transport_str(char * str)
{
    int i,j,state;
    char num[64], tag[64];

    j=0;
    i=0;
    state = REACH_TAG;

    while (1)
    {
        switch (state)
        {
          case REACH_TAG:
            switch (str[i])
            {
              case ';':
              case ',':
              case '=':
              case '\n':
                tag[j++] = '\0';
                j = 0;
                if (!strcmp(tag, "port"))
                {
                  num[j++] = '0';  /* avoid empty fields */
                  state = REACH_PORT;
                }
                else if (!strcmp(tag, "client_port"))
                {
                  num[j++] = '0';  /* avoid empty fields */
                  state = REACH_CLPORT;
                }
                else if (!strcmp(tag, "server_port"))
                {
                  num[j++] = '0';  /* avoid empty fields */
                  state = REACH_SRVPORT;
                }
                else if (!strcmp(tag, "ttl"))
                {
                  num[j++] = '0';  /* avoid empty fields */
                  state = REACH_TTL;
                }
                else if (!strcmp(tag, "destination"))
                {
                  state = REACH_DEST;
                }
                else if (!strcmp(tag, "source"))
                {
                  state = REACH_SRC;
                }
                else if (!strcmp(tag, "ssrc"))
                {
                  num[j++] = '0';  /* avoid empty fields */
                  state = REACH_SSRC;
                }
                break;
              default:
                tag[j++] = islower(str[i]);
                j %= 64; /* avoid illegal access */
                break;
            }
            break;
          case REACH_PORT:
            if (isdigit(str[i]))
            {
              num[j++] = str[i];
              j %= 64; /* avoid illegal access */
            }
            else
            {
              num[j++] = '\0';
              player->rtp_server_port = atoi(num);
              j = 0;
              state = REACH_TAG; /* get only first value */
            }
            break;
          case REACH_CLPORT:
            if (isdigit(str[i]))
            {
              num[j++] = str[i];
              j %= 64; /* avoid illegal access */
            }
            else
            {
              num[j++] = '\0';
              player->rtp_client_port = atoi(num);
              j = 0;
              state = REACH_TAG; /* get only first value */
            }
            break;
          case REACH_SRVPORT:
            if (isdigit(str[i]))
            {
              num[j++] = str[i];
              j %= 64; /* avoid illegal access */
            }
            else
            {
              num[j++] = '\0';
              player->rtp_server_port = atoi(num);
              j = 0;
              state = REACH_TAG; /* get only first value */
            }
            break;
          case REACH_TTL:
            if (isdigit(str[i]))
            {
              num[j++] = str[i];
              j %= 64; /* avoid illegal access */
            }
            else
            {
              num[j++] = '\0';
              player->rtp_ttl = atoi(num);
              j = 0;
              state = REACH_TAG;
            }
            break;
          case REACH_DEST:
            if (isdigit(str[i]) ||
                str[i] == '.')
            {
              num[j++] = str[i];
              j %= 64; /* avoid illegal access */
            }
            else if(str[i] == ':')
            {
              num[j++] = '\0';
              /* convert IP string to NGuint */
              inet_aton(num, &player->rtp_ip);
              j = 0;
              state = REACH_CLPORT;
            }
            else
            {
              num[j++] = '\0';
              /* convert IP string to NGuint */
              inet_aton(num, &player->rtp_ip);
              j = 0;
              state = REACH_TAG;
            }
            break;
          case REACH_SRC:
            if (isdigit(str[i]) ||
                str[i] == '.')
            {
              num[j++] = str[i];
              j %= 64; /* avoid illegal access */
            }
            else
            {
              num[j++] = '\0';
              /* convert IP string to NGuint */
              inet_aton(num, &player->rtp_ip);
              j = 0;
              state = REACH_TAG;
            }
            break;
          case REACH_SSRC:
            break;
        }
        i++;
        /* process terminating NULL so the last tag is correctly detected */
        if (str[i-1]=='\0')
          break;
    }
}

BOOL rtsp_open (U16 PortNo,char* server)
{

  int ret;
  /*char * temp;*/
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  session_desc_t *sdp;
  media_desc_t *media;
  sdp_decode_info_t *sdpdecode;
  rtsp_session_t *session;
  int dummy;
  char transport_buf[200];

  memset(&cmd, 0, sizeof(rtsp_command_t));
  printf("begin to rtsp_create_client\n");
  rtsp_client = rtsp_create_client(server, &ret);

  if (rtsp_client == NULL)
  {
    printf("No client created - error %d\n", ret);
    return FALSE;
  }

  if (rtsp_send_describe(rtsp_client, &cmd, &decode) != RTSP_RESPONSE_GOOD)
  {
    printf("Describe response not good\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return FALSE;
  }

  sdpdecode = set_sdp_decode_from_memory(decode->body);
  if (sdpdecode == NULL)
  {
    printf("Couldn't get sdp decode\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return FALSE;
  }

  if (sdp_decode(sdpdecode, &sdp, &dummy) != 0)
  {
    printf("Couldn't decode sdp\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return FALSE;
  }
  free(sdpdecode);

  convert_relative_urls_to_absolute (sdp, server);

  free_decode_response(decode);
  decode = NULL;
  
  sprintf(transport_buf,transport_str,PortNo,PortNo+1);

  cmd.transport = transport_buf;

  media = sdp->media;
  dummy = rtsp_send_setup(rtsp_client,
			  media->control_string,
			  &cmd,
			  &session,
			  &decode, 0);

  if (dummy != RTSP_RESPONSE_GOOD)
  {
    printf("Response to setup is %d\n", dummy);
    sdp_free_session_desc(sdp);
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return FALSE;
  }

  rtsp_client->session=strdup(session->session);

  player=malloc(sizeof(player_t));
  memset(player, 0, sizeof(player_t));
  if(player==NULL)
   {    
    sdp_free_session_desc(sdp);
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return FALSE;
  }
  
  media = (sdp->media)->next;
  parse_transport_str(decode->transport );

  while(media!=NULL)
  {
    free_decode_response(decode);
    sprintf(transport_buf,transport_str,player->rtp_client_port,player->rtp_client_port  +  1);
    cmd.transport = transport_buf;
    dummy = rtsp_send_setup(rtsp_client,
			  media->control_string,
			  &cmd,
			  &session,
			  &decode, 1);

    if (dummy != RTSP_RESPONSE_GOOD)
    {
      printf("Response to setup is %d\n", dummy);
      sdp_free_session_desc(sdp);
      free_decode_response(decode);
      free_rtsp_client(rtsp_client);
      return FALSE;
    }
    media = media->next;
  }

  parse_transport_str( decode->transport );

  sdp_free_session_desc(sdp);
  free_decode_response(decode);
  cmd.transport = NULL;
  player->state=SETUP;

  return TRUE;
}

BOOL rtsp_play(U32 npt)
{
    rtsp_command_t cmd;
  rtsp_decode_t *decode;

  int dummy;
  if(player->state & PAUSE)
  {
    memset(&cmd, 0, sizeof(rtsp_command_t));
    cmd.transport = NULL;
    cmd.range = "npt=0.0-";
    dummy = rtsp_send_aggregate_play(rtsp_client,rtsp_client->url, &cmd, &decode);
    if (dummy != RTSP_RESPONSE_GOOD)
    {
      printf("response to play is %d\n", dummy);
    }
    free_decode_response(decode);

    player->state=PLAY;
  }
  else
  {
    memset(&cmd, 0, sizeof(rtsp_command_t));
    cmd.transport = NULL;
    sprintf(cmd.range,  "npt=%u-", npt);
    dummy = rtsp_send_aggregate_play(rtsp_client,rtsp_client->url, &cmd, &decode);  
    if (dummy != RTSP_RESPONSE_GOOD)
    {
      printf("response to play is %d\n", dummy);
    }

    free_decode_response(decode);

    if(player->rtp_client_port!=0)
    {
        printf("ip tv start \n");
    }
    player->state=PLAY;
  }
  return TRUE;
}

BOOL rtsp_pause(void)
{
    rtsp_command_t cmd;
    rtsp_decode_t *decode;

    int dummy;
    memset(&cmd, 0, sizeof(rtsp_command_t));
    cmd.transport = NULL;

    dummy = rtsp_send_aggregate_pause(rtsp_client,rtsp_client->url, &cmd, &decode);
    if (dummy != RTSP_RESPONSE_GOOD)
    {
    printf("response to play is %d\n", dummy);
    return FALSE;
    }

    free_decode_response(decode);
    player->state =PAUSE ;
    return TRUE;

}

U32 rtsp_getparam(void)
{
    rtsp_command_t cmd;
    rtsp_decode_t *decode;

    int dummy;
    memset(&cmd, 0, sizeof(rtsp_command_t));
    cmd.transport = NULL;

    dummy = rtsp_send_get_parameter(rtsp_client,rtsp_client->url, &cmd, &decode);
    if (dummy != RTSP_RESPONSE_GOOD)
    {
    printf("response to play is %d\n", dummy);
    return FALSE;
    }

    free_decode_response(decode);
    return TRUE;
	
}

BOOL rtsp_stop(void)
{
    rtsp_decode_t *decode;

    int dummy;
    dummy = rtsp_send_aggregate_teardown(rtsp_client,rtsp_client->url, NULL, &decode);
    printf("Teardown response %d\n", dummy);
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    rtsp_client=NULL;

    if(player->state && (PLAY|PAUSE))
    {
    printf("ip tv stop \n");
    }
    player->state=0;
    free(player);
    return TRUE;
}

void rtsp_print(const char* fmt, ...)
{
#if 1
	printf(fmt);
#endif
}
