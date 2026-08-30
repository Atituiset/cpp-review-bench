// AUTO-DRAFT from curl/curl PR #15289
#include <curl/curl.h>

#include "urldata.h"
#include "warnless.h"
#include "escape.h"
/* Escapes for URL the given unescaped string of given length.
 * 'data' is ignored since 7.82.0.
 */
char *curl_easy_escape(struct Curl_easy *data, const char *string,
                       int inlength)
{
  size_t length;
 * If olen == NULL, no output length is stored.
 * 'data' is ignored since 7.82.0.
 */
char *curl_easy_unescape(struct Curl_easy *data, const char *string,
                         int length, int *olen)
{
  char *str = NULL;
