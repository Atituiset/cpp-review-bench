// AUTO-DRAFT from curl/curl PR #4307
bool is_connect,
                                 Curl_send_buffer *req_buffer);
CURLcode Curl_http_compile_trailers(struct curl_slist *trailers,
                                    Curl_send_buffer *buffer,  // <<< BUG ANCHOR
                                    struct Curl_easy *handle);

/* protocol-specific functions set up to be called by the main engine */
