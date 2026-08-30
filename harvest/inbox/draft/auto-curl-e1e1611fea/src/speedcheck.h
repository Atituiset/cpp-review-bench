// AUTO-DRAFT from curl/curl PR #15289
#include "curl_setup.h"

#include "timeval.h"

void Curl_speedinit(struct Curl_easy *data);
CURLcode Curl_speedcheck(struct Curl_easy *data,
                         struct curltime now);
