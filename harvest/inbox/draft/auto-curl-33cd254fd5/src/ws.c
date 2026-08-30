// AUTO-DRAFT from curl/curl PR #15289
return (ssize_t)nread;
}

CURL_EXTERN CURLcode curl_ws_recv(struct Curl_easy *data, void *buffer,
                                  size_t buflen, size_t *nread,
                                  const struct curl_ws_frame **metap)
{
  struct connectdata *conn = data->conn;
  struct websocket *ws;
  struct ws_collect ctx;
  return CURLE_OK;
}

static CURLcode ws_send_raw_blocking(CURL *data, struct websocket *ws,
                                     const char *buffer, size_t buflen)
{
  CURLcode result = CURLE_OK;
  size_t nwritten;

  (void)ws;
  while(buflen) {
  return result;
}

static CURLcode ws_send_raw(CURL *data, const void *buffer,
                            size_t buflen, size_t *pnwritten)
{
  struct websocket *ws = data->conn->proto.ws;
  return result;
}

CURL_EXTERN CURLcode curl_ws_send(CURL *data, const void *buffer,
                                  size_t buflen, size_t *sent,
                                  curl_off_t fragsize,
                                  unsigned int flags)
  ssize_t n;
  size_t space, payload_added;
  CURLcode result;

  CURL_TRC_WS(data, "curl_ws_send(len=%zu, fragsize=%" FMT_OFF_T
              ", flags=%x), raw=%d",
  return CURLE_OK;
}

CURL_EXTERN const struct curl_ws_frame *curl_ws_meta(struct Curl_easy *data)
{
  /* we only return something for websocket, called from within the callback
     when not using raw mode */
  if(GOOD_EASY_HANDLE(data) && Curl_is_in_callback(data) && data->conn &&
     data->conn->proto.ws && !data->set.ws_raw_mode)
    return &data->conn->proto.ws->frame;
  return CURLE_NOT_BUILT_IN;
}

CURL_EXTERN const struct curl_ws_frame *curl_ws_meta(struct Curl_easy *data)
{
  (void)data;
  return NULL;
