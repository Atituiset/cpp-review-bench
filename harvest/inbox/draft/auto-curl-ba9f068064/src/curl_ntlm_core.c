// AUTO-DRAFT from curl/curl PR #15289
ascii_uppercase_to_unicode_le(identity, user, userlen);
  ascii_to_unicode_le(identity + (userlen << 1), domain, domlen);

  result = Curl_hmacit(Curl_HMAC_MD5, ntlmhash, 16, identity, identity_len,
                       ntlmv2hash);
  free(identity);


  /* Concatenate the Type 2 challenge with the BLOB and do HMAC MD5 */
  memcpy(ptr + 8, &ntlm->nonce[0], 8);
  result = Curl_hmacit(Curl_HMAC_MD5, ntlmv2hash, HMAC_MD5_LENGTH, ptr + 8,
                    NTLMv2_BLOB_LEN + 8, hmac_output);
  if(result) {
    free(ptr);
  memcpy(&data[0], challenge_server, 8);
  memcpy(&data[8], challenge_client, 8);

  result = Curl_hmacit(Curl_HMAC_MD5, ntlmv2hash, 16, &data[0], 16,
                       hmac_output);
  if(result)
    return result;
