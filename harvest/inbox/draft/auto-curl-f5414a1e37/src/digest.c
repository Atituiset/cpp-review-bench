// AUTO-DRAFT from curl/curl PR #15289
return result;

  /* So far so good, now calculate A1 and H(A1) according to RFC 2831 */
  ctxt = Curl_MD5_init(Curl_DIGEST_MD5);
  if(!ctxt)
    return CURLE_OUT_OF_MEMORY;

                  curlx_uztoui(strlen(passwdp)));
  Curl_MD5_final(ctxt, digest);

  ctxt = Curl_MD5_init(Curl_DIGEST_MD5);
  if(!ctxt)
    return CURLE_OUT_OF_MEMORY;

    return CURLE_OUT_OF_MEMORY;

  /* Calculate H(A2) */
  ctxt = Curl_MD5_init(Curl_DIGEST_MD5);
  if(!ctxt) {
    free(spn);

    msnprintf(&HA2_hex[2 * i], 3, "%02x", digest[i]);

  /* Now calculate the response hash */
  ctxt = Curl_MD5_init(Curl_DIGEST_MD5);
  if(!ctxt) {
    free(spn);
