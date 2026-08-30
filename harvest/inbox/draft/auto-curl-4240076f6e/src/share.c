// AUTO-DRAFT from curl/curl PR #15289
#include "curl_memory.h"
#include "memdebug.h"

struct Curl_share *
curl_share_init(void)
{
  struct Curl_share *share = calloc(1, sizeof(struct Curl_share));

#undef curl_share_setopt
CURLSHcode
curl_share_setopt(struct Curl_share *share, CURLSHoption option, ...)
{
  va_list param;
  int type;
  curl_lock_function lockfunc;
  curl_unlock_function unlockfunc;
  void *ptr;
  CURLSHcode res = CURLSHE_OK;

  if(!GOOD_SHARE_HANDLE(share))
    return CURLSHE_INVALID;
}

CURLSHcode
curl_share_cleanup(struct Curl_share *share)
{
  if(!GOOD_SHARE_HANDLE(share))
    return CURLSHE_INVALID;
