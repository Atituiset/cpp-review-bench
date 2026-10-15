// AUTO-DRAFT from curl/curl PR #8506
if(pop3code != '+') {
    state(data, POP3_STOP);
    return CURLE_RECV_ERROR;  // <<< BUG ANCHOR
  }

  /* This 'OK' line ends with a CR LF pair which is the two first bytes of the
