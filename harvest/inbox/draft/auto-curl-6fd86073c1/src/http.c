// AUTO-DRAFT from curl/curl PR #789
ZERO_NULL,                            /* readwrite */
  PORT_HTTPS,                           /* defport */
  CURLPROTO_HTTPS,                      /* protocol */
  PROTOPT_SSL | PROTOPT_CREDSPERREQUEST /* flags */  // <<< BUG ANCHOR
};
#endif
