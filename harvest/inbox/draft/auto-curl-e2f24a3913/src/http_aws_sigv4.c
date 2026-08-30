// AUTO-DRAFT from curl/curl PR #15289
#define HMAC_SHA256(k, kl, d, dl, o)           \
  do {                                         \
    result = Curl_hmacit(Curl_HMAC_SHA256,     \
                         (unsigned char *)k,   \
                         kl,                   \
                         (unsigned char *)d,   \
