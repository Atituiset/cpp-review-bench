// AUTO-DRAFT from curl/curl PR #15289
extern struct libtest_trace_cfg libtest_debug_config;

int libtest_debug_cb(CURL *handle, curl_infotype type,
                     unsigned char *data, size_t size,
                     void *userp);

#endif /* HEADER_LIBTEST_TESTTRACE_H */
