// AUTO-DRAFT from curl/curl PR #20545
char method[]     = "AUTHENTICATE";
  char qop[]        = DIGEST_QOP_VALUE_STRING_AUTH;
  char *spn         = NULL;
  // <<< BUG ANCHOR
  /* Decode the challenge message */
  CURLcode result = auth_decode_digest_md5_message(chlg,
  for(i = 0; i < MD5_DIGEST_LEN; i++)
    curl_msnprintf(&resp_hash_hex[2 * i], 3, "%02x", digest[i]);

  /* Generate the response */
  response = curl_maprintf("username=\"%s\",realm=\"%s\",nonce=\"%s\","
                           "cnonce=\"%s\",nc=\"%s\",digest-uri=\"%s\","
                           "response=%s,qop=%s",
                           userp, realm, nonce,
                           cnonce, nonceCount, spn, resp_hash_hex, qop);
  curlx_free(spn);
  if(!response)
    return CURLE_OUT_OF_MEMORY;
