// AUTO-DRAFT from curl/curl PR #6193
Curl_dyn_reset(&s->rcvbuf);
  }
  s->tunnel_state = TUNNEL_INIT;
  s->keepon = TRUE;
  s->cl = 0;
  s->close_connection = FALSE;
  return CURLE_OK;
          return CURLE_ABORTED_BY_CALLBACK;
  // <<< BUG ANCHOR
        if(result) {
          s->keepon = FALSE;
          break;
        }
        else if(gotbytes <= 0) {
            error = SELECT_ERROR;
            failf(data, "Proxy CONNECT aborted");
          }
          s->keepon = FALSE;
          break;
        }

        if(s->keepon > TRUE) {
          /* This means we are currently ignoring a response-body */

          if(s->cl) {
            /* A Content-Length based body: simply count down the counter
               and make sure to break out of the loop when we're done! */
            s->cl--;
            if(s->cl <= 0) {
              s->keepon = FALSE;
              s->tunnel_state = TUNNEL_COMPLETE;
              break;
            }
            if(r == CHUNKE_STOP) {
              /* we're done reading chunks! */
              infof(data, "chunk reading DONE\n");
              s->keepon = FALSE;
              /* we did the full CONNECT treatment, go COMPLETE */
              s->tunnel_state = TUNNEL_COMPLETE;
            }
            /* If we get a 407 response code with content length
               when we have no auth problem, we must ignore the
               whole response-body */
            s->keepon = 2;

            if(s->cl) {
              infof(data, "Ignore %" CURL_FORMAT_CURL_OFF_T
              if(r == CHUNKE_STOP) {
                /* we're done reading chunks! */
                infof(data, "chunk reading DONE\n");
                s->keepon = FALSE;
                /* we did the full CONNECT treatment, go to COMPLETE */
                s->tunnel_state = TUNNEL_COMPLETE;
              }
              /* without content-length or chunked encoding, we
                 can't keep the connection alive since the close is
                 the end signal so we bail out at once instead */
              s->kee
