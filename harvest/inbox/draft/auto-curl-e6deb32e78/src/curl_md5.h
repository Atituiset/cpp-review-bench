// AUTO-DRAFT from curl/curl PR #15289
#define MD5_DIGEST_LEN  16

typedef CURLcode (* Curl_MD5_init_func)(void *context);
typedef void (* Curl_MD5_update_func)(void *context,
                                      const unsigned char *data,
                                      unsigned int len);
typedef void (* Curl_MD5_final_func)(unsigned char *result, void *context);

struct MD5_params {
  Curl_MD5_init_func     md5_init_func;   /* Initialize context procedure */
  void                  *md5_hashctx;   /* Hash function context */
};

extern const struct MD5_params Curl_DIGEST_MD5[1];
extern const struct HMAC_params Curl_HMAC_MD5[1];

CURLcode Curl_md5it(unsigned char *output, const unsigned char *input,
                    const size_t len);
