// AUTO-DRAFT from curl/curl PR #20545
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define DIGEST_QOP_VALUE_STRING_AUTH      "auth"
/* …（同文件无关代码省略）… */
static bool auth_digest_get_key_value(const char *chlg, const char *key,
                                      char *buf, size_t buflen)
{
  /* keyword=[value],keyword2=[value]
     The values may or may not be quoted.
   */

  do {
    struct Curl_str data;
    struct Curl_str name;

    curlx_str_passblanks(&chlg);

    if(!curlx_str_until(&chlg, &name, 64, '=') &&
       !curlx_str_single(&chlg, '=')) {
      /* this is the key, get the value, possibly quoted */
      int rc = curlx_str_quotedword(&chlg, &data, 256);
      if(rc == STRE_BEGQUOTE)
        /* try unquoted until comma */
        rc = curlx_str_until(&chlg, &data, 256, ',');
      if(rc)
        return FALSE; /* weird */

      if(curlx_str_cmp(&name, key)) {
        /* if this is our key, return the value */
        size_t len = curlx_strlen(&data);
        const char *src = curlx_str(&data);
        size_t i;
        size_t outlen = 0;

        if(len >= buflen)
          /* does not fit */
          return FALSE;

        for(i = 0; i < len; i++) {
          if(src[i] == '\\' && i + 1 < len) {
            i++; /* skip backslash */
          }
          buf[outlen++] = src[i];
        }
        buf[outlen] = 0;
        return TRUE;
      }
      if(curlx_str_single(&chlg, ','))
        return FALSE;
    }
    else /* odd syntax */
      break;
  } while(1);

  return FALSE;
}
/* …（同文件无关代码省略）… */
static CURLcode auth_decode_digest_md5_message(const struct bufref *chlgref,
                                               char *nonce, size_t nlen,
                                               char *realm, size_t rlen,
                                               char *alg, size_t alen,
                                               char *qop, size_t qlen)
{
  const char *chlg = Curl_bufref_ptr(chlgref);

  /* Ensure we have a valid challenge message */
  if(!Curl_bufref_len(chlgref))
    return CURLE_BAD_CONTENT_ENCODING;

  /* Retrieve nonce string from the challenge */
  if(!auth_digest_get_key_value(chlg, "nonce", nonce, nlen))
    return CURLE_BAD_CONTENT_ENCODING;

  /* Retrieve realm string from the challenge */
  if(!auth_digest_get_key_value(chlg, "realm", realm, rlen)) {
    /* Challenge does not have a realm, set empty string [RFC2831] page 6 */
    *realm = '\0';
  }

  /* Retrieve algorithm string from the challenge */
  if(!auth_digest_get_key_value(chlg, "algorithm", alg, alen))
    return CURLE_BAD_CONTENT_ENCODING;

  /* Retrieve qop-options string from the challenge */
  if(!auth_digest_get_key_value(chlg, "qop", qop, qlen))
    return CURLE_BAD_CONTENT_ENCODING;

  return CURLE_OK;
}
/* …（同文件无关代码省略）… */
  char method[]     = "AUTHENTICATE";
  char qop[]        = DIGEST_QOP_VALUE_STRING_AUTH;
  char *spn         = NULL;

  /* Decode the challenge message */
  CURLcode result = auth_decode_digest_md5_message(chlg,
/* …（同文件无关代码省略）… */
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
