// AUTO-DRAFT from curl/curl PR #15289
#include <curl/curl.h>

#include "formdata.h"
#if !defined(CURL_DISABLE_HTTP) && !defined(CURL_DISABLE_FORM_API)

 * a NULL pointer in the 'data' argument.
 */

CURLcode Curl_getformdata(struct Curl_easy *data,
                          curl_mimepart *finalform,
                          struct curl_httppost *post,
                          curl_read_callback fread_func)
