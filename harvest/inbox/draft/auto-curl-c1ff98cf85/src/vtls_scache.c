// AUTO-DRAFT from curl/curl PR #0dc22b690dd8dba4048d494f09a50122dd7c0dd4
{
  struct Curl_ssl_session *s = obj;
  (void)udata;
  curlx_free(CURL_UNCONST(s->sdata));  // <<< BUG ANCHOR
  curlx_free(CURL_UNCONST(s->quic_tp));
  curlx_free((void *)s->alpn);
  curlx_free(s);
}
