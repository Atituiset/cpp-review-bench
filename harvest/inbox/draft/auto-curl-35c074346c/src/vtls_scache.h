// AUTO-DRAFT from curl/curl PR #0dc22b690dd8dba4048d494f09a50122dd7c0dd4
/* All about an SSL session ticket */
struct Curl_ssl_session {
  const void *sdata;           /* session ticket data, plain bytes */  // <<< BUG ANCHOR
  size_t sdata_len;            /* number of bytes in sdata */
  curl_off_t valid_until;      /* seconds since EPOCH until ticket expires */
  int ietf_tls_id;             /* TLS protocol identifier negotiated */
  char *alpn;                  /* APLN TLS negotiated protocol string */
  size_t earlydata_max;        /* max 0-RTT data supported by peer */
  const unsigned char *quic_tp; /* Optional QUIC transport param bytes */
  size_t quic_tp_len;          /* number of bytes in quic_tp */
  struct Curl_llist_node list; /*  internal storage handling */
  BIT(sectrust_verified);      /* session comes from sectrust verified TLS */
