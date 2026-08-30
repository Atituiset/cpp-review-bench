// AUTO-DRAFT from curl/curl PR #15289
}


static size_t write_callback(void *ptr, size_t size, size_t nmemb,
                             void *stream)
{
  int amount = curlx_uztosi(size * nmemb);
