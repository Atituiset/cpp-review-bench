// AUTO-DRAFT from curl/curl PR #15289
}

static size_t
doh_write_cb(const void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct dynbuf *mem = (struct dynbuf *)userp;
  return 0;
}

#define ERROR_CHECK_SETOPT(x,y) \
do {                                          \
  result = curl_easy_setopt(doh, x, y);       \
  if(result &&                                \
     result != CURLE_NOT_BUILT_IN &&          \
     result != CURLE_UNKNOWN_OPTION)          \
    goto error;                               \
} while(0)

static CURLcode doh_run_probe(struct Curl_easy *data,
                              struct doh_probe *p, DNStype dnstype,
  ERROR_CHECK_SETOPT(CURLOPT_PROTOCOLS, CURLPROTO_HTTP|CURLPROTO_HTTPS);
#endif
  ERROR_CHECK_SETOPT(CURLOPT_TIMEOUT_MS, (long)timeout_ms);
  ERROR_CHECK_SETOPT(CURLOPT_SHARE, data->share);
  if(data->set.err && data->set.err != stderr)
    ERROR_CHECK_SETOPT(CURLOPT_STDERR, data->set.err);
  if(Curl_trc_ft_is_verbose(data, &Curl_doh_trc))
