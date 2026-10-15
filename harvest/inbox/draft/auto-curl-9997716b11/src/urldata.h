// AUTO-DRAFT from curl/curl PR #789
bool multiplex; /* connection is multiplexed */
  // <<< BUG ANCHOR
  bool tcp_fastopen; /* use TCP Fast Open */
};

struct hostname {
                                        url query strings (?foo=bar) ! */
#define PROTOPT_CREDSPERREQUEST (1<<7) /* requires login credentials per
                                          request instead of per connection */


/* return the count of bytes sent, or -1 on error */
typedef ssize_t (Curl_send)(struct connectdata *conn, /* connection data */

  size_t maxconnects;  /* Max idle connections in the connection cache */

  bool ssl_enable_npn;  /* TLS NPN extension? */
  bool ssl_enable_alpn; /* TLS ALPN extension? */
  bool path_as_is;      /* allow dotdots? */
  bool pipewait;        /* wait for pipe/multiplex status before starting a
                           new connection */
