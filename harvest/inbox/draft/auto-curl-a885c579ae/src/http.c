// AUTO-DRAFT from curl/curl PR #17753
static CURLcode http_header_s(struct Curl_easy *data,
                              const char *hd, size_t hdlen)
{
  struct connectdata *conn = data->conn;
  const char *v;

#if !defined(CURL_DISABLE_COOKIES)
  v = (data->cookies && data->state.cookie_engine) ?
