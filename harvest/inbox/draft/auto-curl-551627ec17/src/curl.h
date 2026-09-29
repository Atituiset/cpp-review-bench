// AUTO-DRAFT from curl/curl PR #15289
*/
  // <<< BUG ANCHOR
#ifdef CURL_NO_OLDIES
#define CURL_STRICTER
#endif

/* Compile-time deprecation macros. */
extern "C" {
#endif

#if defined(BUILDING_LIBCURL) || defined(CURL_STRICTER)
typedef struct Curl_easy CURL;
typedef struct Curl_share CURLSH;
#else
typedef void CURL;
typedef void CURLSH;
#endif

/*
 * libcurl external API function linkage decorations.
