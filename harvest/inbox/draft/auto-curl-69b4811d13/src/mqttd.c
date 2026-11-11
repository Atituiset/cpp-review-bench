// AUTO-DRAFT from curl/curl PR #23007
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
  // <<< BUG ANCHOR
#define MQTT_MSG_CONNECT    0x10
/* …（同文件无关代码省略）… */
typedef enum {
  FROM_CLIENT,
  FROM_SERVER
} mqttdir;
/* …（同文件无关代码省略）… */
static void logprotocol(mqttdir dir,
                        const char *prefix, size_t remlen,
                        FILE *output,
                        const unsigned char *buffer, ssize_t len)
{
  char data[12000] = "";
  ssize_t i;
  const unsigned char *ptr = buffer;
  char *optr = data;
  int left = sizeof(data);

  for(i = 0; i < len && (left >= 0); i++) {
    snprintf(optr, left, "%02x", ptr[i]);
    optr += 2;
    left -= 2;
  }
  fprintf(output, "%s %s %x %s\n",
          dir == FROM_CLIENT ? "client" : "server",
          prefix, (unsigned int)remlen, data);
}
/* …（同文件无关代码省略）… */
static size_t decode_length(const unsigned char *buffer,
                            size_t buflen, size_t *lenbytes)
{
  size_t len = 0;
  size_t mult = 1;
  size_t i;
  unsigned char encoded = 0x80;

  for(i = 0; (i < buflen) && (encoded & 0x80); i++) {
    encoded = buffer[i];
    len += (encoded & 0x7f) * mult;
    mult *= 0x80;
  }

  if(lenbytes)
    *lenbytes = i;

  return len;
}
/* …（同文件无关代码省略）… */
#define MAX_TOPIC_LENGTH     65535
/* …（同文件无关代码省略）… */

static char topic[MAX_TOPIC_LENGTH + 1];

static bool fixedheader(curl_socket_t fd,
                        unsigned char *bytep,
                        size_t *remaining_lengthp,
                        size_t *remaining_length_bytesp)
{
  /* get the fixed header */
  unsigned char buffer[10];

  /* get the first two bytes */
  ssize_t rc = sread(fd, buffer, 2);
  size_t i;
  if(rc < 2) {
    logmsg("READ %zd bytes [SHORT!]", rc);
    return FALSE; /* fail */
  }
  logmsg("READ %zd bytes", rc);
  loghex(buffer, rc);
  *bytep = buffer[0];

  /* if the length byte has the top bit set, get the next one too */
  i = 1;
  while(buffer[i] & 0x80) {
    i++;
    rc = sread(fd, &buffer[i], 1);
    if(rc != 1) {
      logmsg("Remaining Length broken");
      return FALSE;
    }
  }
  *remaining_lengthp = decode_length(&buffer[1], i, remaining_length_bytesp);
  logmsg("Remaining Length: %zu [%zu bytes]", *remaining_lengthp,
         *remaining_length_bytesp);
  return TRUE;
}
/* …（同文件无关代码省略）… */
      buffer = newbuffer;
    }

    if(remaining_length) {
      /* reading variable header and payload into buffer */
      rc = sread(fd, buffer, remaining_length);
      if(rc > 0) {
        logmsg("READ %zd bytes", rc);
        loghex(buffer, rc);
      }
    }

    if(byte == MQTT_MSG_CONNECT) {
      logprotocol(FROM_CLIENT, "CONNECT", remaining_length, dump, buffer, rc);
/* …（同文件无关代码省略）… */
#endif
      /* expect a disconnect here */
      /* get the request */
      rc = sread(fd, &buffer[0], 2);

      logmsg("READ %zd bytes [DISCONNECT]", rc);
      loghex(buffer, rc);
      logprotocol(FROM_CLIENT, "DISCONNECT", 0, dump, buffer, rc);
      goto end;
    }
    else {
