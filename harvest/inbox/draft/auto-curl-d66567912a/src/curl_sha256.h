// AUTO-DRAFT from curl/curl PR #15289
#include <curl/curl.h>
#include "curl_hmac.h"

extern const struct HMAC_params Curl_HMAC_SHA256[1];

#ifndef CURL_SHA256_DIGEST_LENGTH
#define CURL_SHA256_DIGEST_LENGTH 32 /* fixed size */
