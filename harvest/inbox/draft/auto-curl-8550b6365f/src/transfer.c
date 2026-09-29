// AUTO-DRAFT from curl/curl PR #5045
*                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
  else
    fd_write = CURL_SOCKET_BAD;
  // <<< BUG ANCHOR
  if(data->state.drain) {
    data->state.drain--;
    select_res |= CURL_CSELECT_IN;
    DEBUGF(infof(data, "Curl_readwrite: forcibly told to drain data\n"));
  }
