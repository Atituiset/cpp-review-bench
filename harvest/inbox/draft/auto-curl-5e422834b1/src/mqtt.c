// AUTO-DRAFT from curl/curl PR #23007
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define MQTT_MSG_PUBLISH    0x30
#define MQTT_MSG_SUBSCRIBE  0x82
/* …（同文件无关代码省略）… */
#define CURL_META_MQTT_EASY   "meta:proto:mqtt:easy"
/* …（同文件无关代码省略）… */
#define CURL_META_MQTT_CONN   "meta:proto:mqtt:conn"
/* …（同文件无关代码省略）… */
enum mqttstate {
  MQTT_FIRST,             /* 0 */
  MQTT_REMAINING_LENGTH,  /* 1 */
  MQTT_CONNACK,           /* 2 */
  MQTT_SUBACK,            /* 3 */
  MQTT_SUBACK_COMING,     /* 4 - the SUBACK remainder */
  MQTT_PUBWAIT,    /* 5 - wait for publish */
  MQTT_PUB_REMAIN,  /* 6 - wait for the remainder of the publish */

  MQTT_NOSTATE /* 7 - never used an actual state */
};

struct mqtt_conn {
/* …（同文件无关代码省略）… */
struct MQTT {
  struct dynbuf sendbuf;
  /* when receiving */
  struct dynbuf recvbuf;
  size_t npacket; /* byte counter */
  size_t remaining_length;
  unsigned char pkt_hd[4]; /* for decoding the arriving packet length */
  struct curltime lastTime; /* last time we sent or received data */
  unsigned char firstbyte;
  BIT(pingsent); /* 1 while we wait for ping response */
};
/* …（同文件无关代码省略）… */
static CURLcode mqtt_send(struct Curl_easy *data,
                          const char *buf, size_t len)
{
  size_t n;
  CURLcode result;
  struct MQTT *mq = Curl_meta_get(data, CURL_META_MQTT_EASY);

  if(!mq)
    return CURLE_FAILED_INIT;

  result = Curl_xfer_send(data, buf, len, FALSE, &n);
  if(result)
    return result;
  mq->lastTime = *Curl_pgrs_now(data);
  Curl_debug(data, CURLINFO_HEADER_OUT, buf, n);
  if(len != n) {
    size_t nsend = len - n;
    if(curlx_dyn_len(&mq->sendbuf)) {
      DEBUGASSERT(curlx_dyn_len(&mq->sendbuf) >= nsend);
      result = curlx_dyn_tail(&mq->sendbuf, nsend); /* keep this much */
    }
    else {
      result = curlx_dyn_addn(&mq->sendbuf, &buf[n], nsend);
    }
  }
  else
    curlx_dyn_reset(&mq->sendbuf);
  return result;
}

/* Generic function called by the multi interface to figure out what socket(s)
   to wait for and for what actions during the DOING and PROTOCONNECT
   states */
static CURLcode mqtt_pollset(struct Curl_easy *data,
                             struct easy_pollset *ps)
{
  return Curl_pollset_add_in(data, ps, data->conn->sock[FIRSTSOCKET]);
}

static int mqtt_encode_len(char *buf, size_t len)
{
  int i;

  for(i = 0; (len > 0) && (i < 4); i++) {
    unsigned char encoded;
    encoded = len % 0x80;
    len /= 0x80;
    if(len)
      encoded |= 0x80;
    buf[i] = (char)encoded;
  }

  return i;
}
/* …（同文件无关代码省略）… */
static CURLcode mqtt_disconnect(struct Curl_easy *data)
{
  return mqtt_send(data, "\xe0\x00", 2);
}
/* …（同文件无关代码省略）… */
static CURLcode mqtt_get_topic(struct Curl_easy *data,
                               char **topic, size_t *topiclen)
{
  const char *path = data->state.up.path;
  CURLcode result = CURLE_URL_MALFORMAT;
  if(strlen(path) > 1) {
    result = Curl_urldecode(path + 1, 0, topic, topiclen, REJECT_CTRL);
    if(!result && (*topiclen > 0xffff)) {
      failf(data, "Too long MQTT topic");
      result = CURLE_URL_MALFORMAT;
    }
  }
  else
    failf(data, "No MQTT topic found. Forgot to URL encode it?");

  return result;
}
/* …（同文件无关代码省略）… */
static CURLcode mqtt_subscribe(struct Curl_easy *data)
{
  CURLcode result = CURLE_OK;
  char *topic = NULL;
  size_t topiclen;
  unsigned char *packet = NULL;
  size_t packetlen;
  char encodedsize[4];
  size_t n;
  struct connectdata *conn = data->conn;
  struct mqtt_conn *mqtt = Curl_conn_meta_get(conn, CURL_META_MQTT_CONN);

  if(!mqtt)
    return CURLE_FAILED_INIT;

  result = mqtt_get_topic(data, &topic, &topiclen);
  if(result)
    goto fail;

  mqtt->packetid++;

  packetlen = topiclen + 5; /* packetid + topic (has a two byte length field)
                               + 2 bytes topic length + QoS byte */
  n = mqtt_encode_len((char *)encodedsize, packetlen);
  packetlen += n + 1; /* add one for the control packet type byte */

  packet = curlx_malloc(packetlen);
  if(!packet) {
    result = CURLE_OUT_OF_MEMORY;
    goto fail;
  }

  packet[0] = MQTT_MSG_SUBSCRIBE;
  memcpy(&packet[1], encodedsize, n);
  packet[1 + n] = (mqtt->packetid >> 8) & 0xff;
  packet[2 + n] = mqtt->packetid & 0xff;
  packet[3 + n] = (topiclen >> 8) & 0xff;
  packet[4 + n] = topiclen & 0xff;
  memcpy(&packet[5 + n], topic, topiclen);
  packet[5 + n + topiclen] = 0; /* QoS zero */

  result = mqtt_send(data, (const char *)packet, packetlen);

fail:
  curlx_free(topic);
  curlx_free(packet);
  return result;
}
/* …（同文件无关代码省略）… */
#define MAX_MQTT_MESSAGE_SIZE 0xFFFFFFF
/* …（同文件无关代码省略）… */
static CURLcode mqtt_publish(struct Curl_easy *data)
{
  CURLcode result;
  char *payload = data->set.postfields;
  size_t payloadlen;
  char *topic = NULL;
  size_t topiclen;
  unsigned char *pkt = NULL;
  size_t i = 0;
  size_t remaininglength;
  size_t encodelen;
  char encodedbytes[4];
  curl_off_t postfieldsize = data->set.postfieldsize;

  if(!payload) {
    DEBUGF(infof(data, "mqtt_publish without payload, return bad arg"));
    return CURLE_BAD_FUNCTION_ARGUMENT;
  }
  if(!curlx_sotouz_fits(postfieldsize, &payloadlen)) {
    if(postfieldsize > 0) /* off_t does not fit into size_t */
      return CURLE_BAD_FUNCTION_ARGUMENT;
    payloadlen = strlen(payload);
  }

  result = mqtt_get_topic(data, &topic, &topiclen);
  if(result)
    goto fail;

  remaininglength = payloadlen + 2 + topiclen;
  encodelen = mqtt_encode_len(encodedbytes, remaininglength);
  if(remaininglength > (MAX_MQTT_MESSAGE_SIZE - encodelen - 1)) {
    result = CURLE_TOO_LARGE;
    goto fail;
  }

  /* add the control byte and the encoded remaining length */
  pkt = curlx_malloc(remaininglength + 1 + encodelen);
  if(!pkt) {
    result = CURLE_OUT_OF_MEMORY;
    goto fail;
  }

  /* assemble packet */
  pkt[i++] = MQTT_MSG_PUBLISH;
  memcpy(&pkt[i], encodedbytes, encodelen);
  i += encodelen;
  pkt[i++] = (topiclen >> 8) & 0xff;
  pkt[i++] = (topiclen & 0xff);
  memcpy(&pkt[i], topic, topiclen);
  i += topiclen;
  memcpy(&pkt[i], payload, payloadlen);
  i += payloadlen;
  result = mqtt_send(data, (const char *)pkt, i);

fail:
  curlx_free(pkt);
  curlx_free(topic);
  return result;
}
/* …（同文件无关代码省略）… */
  "MQTT_SUBACK_COMING",
  "MQTT_PUBWAIT",
  "MQTT_PUB_REMAIN",

  "NOT A STATE"
};
/* …（同文件无关代码省略）… */
static void mqstate(struct Curl_easy *data,
                    enum mqttstate state,
                    enum mqttstate nextstate) /* used if state == FIRST */
{
  struct connectdata *conn = data->conn;
  struct mqtt_conn *mqtt = Curl_conn_meta_get(conn, CURL_META_MQTT_CONN);
  DEBUGASSERT(mqtt);
  if(!mqtt)
    return;
#ifdef DEBUGBUILD
  infof(data, "%s (from %s) (next is %s)",
        statenames[state],
        statenames[mqtt->state],
        (state == MQTT_FIRST) ? statenames[nextstate] : "");
#endif
  mqtt->state = state;
  if(state == MQTT_FIRST)
    mqtt->nextstate = nextstate;
}
/* …（同文件无关代码省略）… */
static CURLcode mqtt_ping(struct Curl_easy *data)
{
  struct MQTT *mq = Curl_meta_get(data, CURL_META_MQTT_EASY);
  CURLcode result = CURLE_OK;
  struct connectdata *conn = data->conn;
  struct mqtt_conn *mqtt = Curl_conn_meta_get(conn, CURL_META_MQTT_CONN);

  if(!mqtt || !mq)
    return CURLE_FAILED_INIT;

  if(mqtt->state == MQTT_FIRST &&
     !mq->pingsent &&
     data->set.upkeep_interval_ms > 0) {
    struct curltime t = *Curl_pgrs_now(data);
    timediff_t diff = curlx_ptimediff_ms(&t, &mq->lastTime);

    if(diff > data->set.upkeep_interval_ms) {
      /* 0xC0 is PINGREQ, and 0x00 is remaining length */
      unsigned char packet[2] = { 0xC0, 0x00 };
      size_t packetlen = sizeof(packet);

      result = mqtt_send(data, (char *)packet, packetlen);
      if(!result) {
        mq->pingsent = TRUE;
      }
      infof(data, "mqtt_ping: sent ping request.");
    }
  }
  return result;
}
/* …（同文件无关代码省略）… */

  if(curlx_dyn_len(&mq->sendbuf)) {
    /* send the remainder of an outgoing packet */
    result = mqtt_send(data, curlx_dyn_ptr(&mq->sendbuf),
                       curlx_dyn_len(&mq->sendbuf));
    if(result)
      return result;
  }

  result = mqtt_ping(data);
  if(result)
    return result;

  infof(data, "mqtt_doing: state [%d]", (int)mqtt->state);
/* …（同文件无关代码省略）… */
    if(result)
      break;

    if(data->state.httpreq == HTTPREQ_POST) {
      result = mqtt_publish(data);
      if(!result) {
        result = mqtt_disconnect(data);
        *done = TRUE;
      }
      mqtt->nextstate = MQTT_FIRST;
    }
    else {
      result = mqtt_subscribe(data);
      if(!result) {
        mqstate(data, MQTT_FIRST, MQTT_SUBACK);
      }
    }
    break;

  case MQTT_SUBACK:
