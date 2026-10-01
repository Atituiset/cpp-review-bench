// AUTO-DRAFT from curl/curl PR #3262
static CURLcode nss_sslver_from_curl(PRUint16 *nssver, long version)
{
  switch(version) {
  case CURL_SSLVERSION_TLSv1:  // <<< BUG ANCHOR
    /* TODO: set sslver->max to SSL_LIBRARY_VERSION_TLS_1_3 once stable */
#ifdef SSL_LIBRARY_VERSION_TLS_1_2
    *nssver = SSL_LIBRARY_VERSION_TLS_1_2;
#elif defined SSL_LIBRARY_VERSION_TLS_1_1
    *nssver = SSL_LIBRARY_VERSION_TLS_1_1;
#else
    *nssver = SSL_LIBRARY_VERSION_TLS_1_0;
#endif
    return CURLE_OK;

  case CURL_SSLVERSION_SSLv2:
    *nssver = SSL_LIBRARY_VERSION_2;
    return CURLE_OK;
  }

  switch(min) {
  case CURL_SSLVERSION_DEFAULT:
    break;
  case CURL_SSLVERSION_TLSv1:
    sslver->min = SSL_LIBRARY_VERSION_TLS_1_0;
    break;
  default:
    result = nss_sslver_from_curl(&sslver->min, min);

  SSLVersionRange sslver = {
    SSL_LIBRARY_VERSION_TLS_1_0,  /* min */
    SSL_LIBRARY_VERSION_TLS_1_0   /* max */
  };

  BACKEND->data = data;
