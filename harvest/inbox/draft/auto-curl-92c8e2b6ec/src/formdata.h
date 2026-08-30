// AUTO-DRAFT from curl/curl PR #15289
bool showfilename_alloc;
};

CURLcode Curl_getformdata(struct Curl_easy *data,
                          curl_mimepart *,
                          struct curl_httppost *post,
                          curl_read_callback fread_func);
