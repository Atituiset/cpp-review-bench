// AUTO-DRAFT from curl/curl PR #15289
#include "warnless.h"
#include "memdebug.h"

size_t WriteOutput(void *ptr, size_t size, size_t nmemb, void *stream);
size_t WriteHeader(void *ptr, size_t size, size_t nmemb, void *stream);

static unsigned long realHeaderSize = 0;

  return res;
}

size_t WriteOutput(void *ptr, size_t size, size_t nmemb, void *stream)
{
  fwrite(ptr, size, nmemb, stream);
  return nmemb * size;
}

size_t WriteHeader(void *ptr, size_t size, size_t nmemb, void *stream)
{
  (void)ptr;
  (void)stream;
