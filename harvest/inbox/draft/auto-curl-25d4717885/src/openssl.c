// AUTO-DRAFT from curl/curl PR #961c95fea6e097ef4dbc19ee996e5127e58acbac
struct ossl_certs_ctx {
  STACK_OF(X509) *sk;
  size_t num_certs;
};

static CURLcode ossl_chain_get_der(struct Curl_cfilter *cf,
  X509 *cert;
  int der_len;

  (void)cf;
  (void)data;
  *pder_len = 0;
  der_len = i2d_X509(cert, pder);
  if(der_len < 0)
    return CURLE_FAILED_INIT;
  *pder_len = (size_t)der_len;
  return CURLE_OK;
}
    result = Curl_vtls_apple_verify(cf, data, peer, chain.num_certs,
                                    ossl_chain_get_der, &chain,
                                    ocsp_data, ocsp_len);
    if(!result && ocsp_missing && conn_config->verifystatus &&
       !octx->reused_session) {
      /* verified, but OCSP stapling is required and server sent none */
