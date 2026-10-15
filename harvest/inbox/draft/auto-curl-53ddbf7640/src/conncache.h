// AUTO-DRAFT from curl/curl PR #15155
*/
CURLcode Curl_cpool_add_pollfds(struct cpool *connc,
                                struct curl_pollfds *cpfds);
CURLcode Curl_cpool_add_waitfds(struct cpool *connc,  // <<< BUG ANCHOR
                                struct curl_waitfds *cwfds);

/**
 * Perform maintenance on connections in the pool. Specifically,
