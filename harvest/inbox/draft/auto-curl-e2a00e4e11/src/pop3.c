// AUTO-DRAFT from curl/curl PR #15289
}

  /* Create the digest */
  ctxt = Curl_MD5_init(Curl_DIGEST_MD5);
  if(!ctxt)
    return CURLE_OUT_OF_MEMORY;
