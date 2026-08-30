// AUTO-DRAFT from curl/curl PR #1727
*                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
                       should be FALSE when it gets to Curl_ftp_quit() */
  bool cwddone;     /* if it has been determined that the proper CWD combo
                       already has been done */
  bool cwdfail;     /* set TRUE if a CWD command fails, as then we must prevent
                       caching the current directory */
  bool wait_data_conn; /* this is set TRUE if data connection is waited */
