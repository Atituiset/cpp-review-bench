// AUTO-DRAFT from curl/curl PR #15289
#include <curl/curl.h>

#include "mime.h"
#include "warnless.h"
#include "urldata.h"
 */

/* Create a mime handle. */
curl_mime *curl_mime_init(struct Curl_easy *easy)
{
  curl_mime *mime;
