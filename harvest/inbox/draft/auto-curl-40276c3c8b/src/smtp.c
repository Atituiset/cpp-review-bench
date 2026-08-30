// AUTO-DRAFT from curl/curl PR #8506
if((smtp->rcpt && smtpcode/100 != 2 && smtpcode != 553 && smtpcode != 1) ||
     (!smtp->rcpt && smtpcode/100 != 2 && smtpcode != 1)) {
    failf(data, "Command failed: %d", smtpcode);
    result = CURLE_RECV_ERROR;  // <<< BUG ANCHOR
  }
  else {
    /* Temporarily add the LF character back and send as body to the client */
  (void)instate; /* no use for this yet */

  if(smtpcode != 250)
    result = CURLE_RECV_ERROR;

  /* End of DONE phase */
  state(data, SMTP_STOP);
