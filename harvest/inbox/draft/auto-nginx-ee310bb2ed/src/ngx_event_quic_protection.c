// AUTO-DRAFT from nginx/nginx PR #631
ngx_quic_secret_t   *client, *server;
    ngx_quic_ciphers_t   ciphers;
  // <<< BUG ANCHOR
    static const uint8_t salt[20] =
        "\x38\x76\x2c\xf7\xf5\x59\x34\xb3\x4d\x17"
        "\x9a\xe6\xa4\xc8\x0c\xad\xcc\xbb\x7f\x0a";

    client = &keys->secrets[ssl_encryption_initial].client;
    server = &keys->secrets[ssl_encryption_initial].server;
    /* 5.8.  Retry Packet Integrity */
    static ngx_quic_md_t  key = ngx_quic_md(
        "\xbe\x0c\x69\x0b\x9f\x66\x57\x5a\x1d\x76\x6b\x54\xe3\x68\xc8\x4e");
    static const u_char   nonce[NGX_QUIC_IV_LEN] =
        "\x46\x15\x99\xd3\x5d\x63\x2b\xf2\x23\x98\x25\xbb";
    static ngx_str_t      in = ngx_string("");

    ad.data = res->data;
