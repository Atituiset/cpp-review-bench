// AUTO-DRAFT from curl/curl PR #1017
#include "memdebug.h"
  // <<< BUG ANCHOR
/*
 * This is supposed to be called in the beginning of a perform() session
 * and should reset all session-info variables
 */
CURLcode Curl_initinfo(struct Curl_easy *data)
{
  info->filetime = -1; /* -1 is an illegal time and thus means unknown */
  info->timecond = FALSE;

  free(info->contenttype);
  info->contenttype = NULL;

  info->header_size = 0;
  info->request_size = 0;
  info->numconnects = 0;

  info->conn_primary_ip[0] = '\0';
  info->conn_local_ip[0] = '\0';
  info->conn_primary_port = 0;
  info->conn_local_port = 0;

  return CURLE_OK;
}
