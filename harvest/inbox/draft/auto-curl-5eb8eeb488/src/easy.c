// AUTO-DRAFT from curl/curl PR #5045
to have this handle checked soon */
  if((newstate & (KEEP_RECV_PAUSE|KEEP_SEND_PAUSE)) !=
     (KEEP_RECV_PAUSE|KEEP_SEND_PAUSE)) {
    data->state.drain++;  // <<< BUG ANCHOR
    Curl_expire(data, 0, EXPIRE_RUN_NOW); /* get this handle going again */
    if(data->multi)
      Curl_update_timer(data->multi);
