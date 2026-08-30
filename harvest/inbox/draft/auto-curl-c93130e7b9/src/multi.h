// AUTO-DRAFT from curl/curl PR #15289
extern "C" {
#endif

#if defined(BUILDING_LIBCURL) || defined(CURL_STRICTER)
typedef struct Curl_multi CURLM;
#else
typedef void CURLM;
#endif

typedef enum {
  CURLM_CALL_MULTI_PERFORM = -1, /* please call curl_multi_perform() or
