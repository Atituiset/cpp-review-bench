// AUTO-DRAFT from curl/curl PR #15289
#include <curl/curl.h>

static size_t cb_curl(void *buffer, size_t size, size_t nmemb, void *userp)
{
  (void)buffer;
  (void)size;
