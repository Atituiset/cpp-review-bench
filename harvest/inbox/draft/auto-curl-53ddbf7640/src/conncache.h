// AUTO-DRAFT from curl/curl PR #15155
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct cpool {
   /* the pooled connections, bundled per destination */
  struct Curl_hash dest2bundle;
  size_t num_conn;
  curl_off_t next_connection_id;
  curl_off_t next_easy_id;
  struct curltime last_cleanup;
  struct Curl_llist shutdowns;  /* The connections being shut down */
  struct Curl_easy *idata; /* internal handle used for discard */
  struct Curl_multi *multi; /* != NULL iff pool belongs to multi */
  struct Curl_share *share; /* != NULL iff pool belongs to share */
  Curl_cpool_disconnect_cb *disconnect_cb;
  BIT(locked);
};
/* …（同文件无关代码省略）… */
 */
CURLcode Curl_cpool_add_pollfds(struct cpool *connc,
                                struct curl_pollfds *cpfds);
CURLcode Curl_cpool_add_waitfds(struct cpool *connc,
                                struct curl_waitfds *cwfds);

/**
 * Perform maintenance on connections in the pool. Specifically,
