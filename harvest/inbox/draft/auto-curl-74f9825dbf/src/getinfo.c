// AUTO-DRAFT from curl/curl PR #1549
*                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
  return CURLE_OK;
}

static CURLcode getinfo_double(struct Curl_easy *data, CURLINFO info,
                               double *param_doublep)
{
  switch(info) {
  case CURLINFO_TOTAL_TIME:
    *param_doublep = data->progress.timespent;
    break;
  case CURLINFO_NAMELOOKUP_TIME:
    *param_doublep = data->progress.t_nslookup;
    break;
  case CURLINFO_CONNECT_TIME:
    *param_doublep = data->progress.t_connect;
    break;
  case CURLINFO_APPCONNECT_TIME:
    *param_doublep = data->progress.t_appconnect;
    break;
  case CURLINFO_PRETRANSFER_TIME:
    *param_doublep =  data->progress.t_pretransfer;
    break;
  case CURLINFO_STARTTRANSFER_TIME:
    *param_doublep = data->progress.t_starttransfer;
    break;
  case CURLINFO_SIZE_UPLOAD:
    *param_doublep =  (double)data->progress.uploaded;
      (double)data->progress.size_ul:-1;
    break;
  case CURLINFO_REDIRECT_TIME:
    *param_doublep =  data->progress.t_redirect;
    break;

  default:
