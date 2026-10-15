// AUTO-DRAFT from curl/curl PR #6193
/* struct for HTTP CONNECT state data */
struct http_connect_state {
  struct dynbuf rcvbuf;
  int keepon;  // <<< BUG ANCHOR
  curl_off_t cl; /* size of content to read and ignore */
  enum {
    TUNNEL_INIT,    /* init/default/no tunnel state */
