// AUTO-DRAFT from curl/curl PR #17753
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>

static CURLcode http_header_s(struct Curl_easy *data,
                              const char *hd, size_t hdlen)
{
  struct connectdata *conn = data->conn;
  const char *v;

#if !defined(CURL_DISABLE_COOKIES)
  v = (data->cookies && data->state.cookie_engine) ?
