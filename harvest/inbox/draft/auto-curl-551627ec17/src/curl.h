// AUTO-DRAFT from curl/curl PR #15289
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>
  // <<< BUG ANCHOR
 */

#ifdef CURL_NO_OLDIES
#define CURL_STRICTER
#endif

/* Compile-time deprecation macros. */
/* …（同文件无关代码省略）… */
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
