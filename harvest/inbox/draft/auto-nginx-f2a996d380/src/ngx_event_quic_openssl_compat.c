// AUTO-DRAFT from nginx/nginx PR #1769
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define NGX_QUIC_COMPAT_SSL_TP_EXT           0x39
/* …（同文件无关代码省略）… */
#define NGX_QUIC_COMPAT_CLIENT_HANDSHAKE     "CLIENT_HANDSHAKE_TRAFFIC_SECRET"
#define NGX_QUIC_COMPAT_SERVER_HANDSHAKE     "SERVER_HANDSHAKE_TRAFFIC_SECRET"
#define NGX_QUIC_COMPAT_CLIENT_APPLICATION   "CLIENT_TRAFFIC_SECRET_0"
#define NGX_QUIC_COMPAT_SERVER_APPLICATION   "SERVER_TRAFFIC_SECRET_0"
/* …（同文件无关代码省略）… */
typedef struct {
    ngx_quic_secret_t             secret;
    ngx_uint_t                    cipher;
} ngx_quic_compat_keys_t;
/* …（同文件无关代码省略）… */
    ngx_str_t *res);


ngx_int_t
ngx_quic_compat_init(ngx_conf_t *cf, SSL_CTX *ctx)
{
    SSL_CTX_set_keylog_callback(ctx, ngx_quic_compat_keylog_callback);

    if (SSL_CTX_has_client_custom_ext(ctx, NGX_QUIC_COMPAT_SSL_TP_EXT)) {
        return NGX_OK;
    }
/* …（同文件无关代码省略）… */
static void
ngx_quic_compat_keylog_callback(const SSL *ssl, const char *line)
{
    u_char                        ch, *p, *start, value;
    size_t                        n;
    ngx_uint_t                    write;
    const SSL_CIPHER             *cipher;
    ngx_quic_compat_t            *com;
    ngx_connection_t             *c;
    ngx_quic_connection_t        *qc;
    enum ssl_encryption_level_t   level;
    u_char                        secret[EVP_MAX_MD_SIZE];

    c = ngx_ssl_get_connection(ssl);
    if (c->type != SOCK_DGRAM) {
        return;
    }

    p = (u_char *) line;

    for (start = p; *p && *p != ' '; p++);

    n = p - start;

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "quic compat secret %*s", n, start);

    if (n == sizeof(NGX_QUIC_COMPAT_CLIENT_HANDSHAKE) - 1
        && ngx_strncmp(start, NGX_QUIC_COMPAT_CLIENT_HANDSHAKE, n) == 0)
    {
        level = ssl_encryption_handshake;
        write = 0;

    } else if (n == sizeof(NGX_QUIC_COMPAT_SERVER_HANDSHAKE) - 1
               && ngx_strncmp(start, NGX_QUIC_COMPAT_SERVER_HANDSHAKE, n) == 0)
    {
        level = ssl_encryption_handshake;
        write = 1;

    } else if (n == sizeof(NGX_QUIC_COMPAT_CLIENT_APPLICATION) - 1
               && ngx_strncmp(start, NGX_QUIC_COMPAT_CLIENT_APPLICATION, n)
                  == 0)
    {
        level = ssl_encryption_application;
        write = 0;

    } else if (n == sizeof(NGX_QUIC_COMPAT_SERVER_APPLICATION) - 1
               && ngx_strncmp(start, NGX_QUIC_COMPAT_SERVER_APPLICATION, n)
                   == 0)
    {
        level = ssl_encryption_application;
        write = 1;

    } else {
        return;
    }

    if (*p++ == '\0') {
        return;
    }

    for ( /* void */ ; *p && *p != ' '; p++);

    if (*p++ == '\0') {
        return;
    }

    for (n = 0, start = p; *p; p++) {
        ch = *p;

        if (ch >= '0' && ch <= '9') {
            value = ch - '0';
            goto next;
        }

        ch = (u_char) (ch | 0x20);

        if (ch >= 'a' && ch <= 'f') {
            value = ch - 'a' + 10;
            goto next;
        }

        ngx_log_error(NGX_LOG_EMERG, c->log, 0,
                      "invalid OpenSSL QUIC secret format");

        return;

    next:

        if ((p - start) % 2) {
            secret[n++] += value;

        } else {
            if (n >= EVP_MAX_MD_SIZE) {
                ngx_log_error(NGX_LOG_EMERG, c->log, 0,
                              "too big OpenSSL QUIC secret");
                return;
            }

            secret[n] = (value << 4);
        }
    }

    qc = ngx_quic_get_connection(c);
    com = qc->compat;
    cipher = SSL_get_current_cipher(ssl);

    if (write) {
        com->method->set_write_secret((SSL *) ssl, level, cipher, secret, n);
        com->write_level = level;

    } else {
        com->method->set_read_secret((SSL *) ssl, level, cipher, secret, n);
        com->read_record = 0;

        if (ngx_quic_compat_set_encryption_secret(c, &com->keys, level,
                                                  cipher, secret, n)
            != NGX_OK)
        {
            qc->error = NGX_QUIC_ERR_INTERNAL_ERROR;
        }
    }

    ngx_explicit_memzero(secret, n);
}
/* …（同文件无关代码省略）… */
static ngx_int_t
ngx_quic_compat_set_encryption_secret(ngx_connection_t *c,
    ngx_quic_compat_keys_t *keys, enum ssl_encryption_level_t level,
    const SSL_CIPHER *cipher, const uint8_t *secret, size_t secret_len)
{
    ngx_int_t            key_len;
    ngx_str_t            secret_str;
    ngx_uint_t           i;
    ngx_quic_md_t        key;
    ngx_quic_hkdf_t      seq[2];
    ngx_quic_secret_t   *peer_secret;
    ngx_quic_ciphers_t   ciphers;
    ngx_pool_cleanup_t  *cln;

    peer_secret = &keys->secret;

    keys->cipher = SSL_CIPHER_get_id(cipher);

    key_len = ngx_quic_ciphers(keys->cipher, &ciphers);

    if (key_len == NGX_ERROR) {
        ngx_ssl_error(NGX_LOG_INFO, c->log, 0, "unexpected cipher");
        return NGX_ERROR;
    }

    key.len = key_len;

    peer_secret->iv.len = NGX_QUIC_IV_LEN;

    secret_str.len = secret_len;
    secret_str.data = (u_char *) secret;

    ngx_quic_hkdf_set(&seq[0], "tls13 key", &key, &secret_str);
    ngx_quic_hkdf_set(&seq[1], "tls13 iv", &peer_secret->iv, &secret_str);

    for (i = 0; i < (sizeof(seq) / sizeof(seq[0])); i++) {
        if (ngx_quic_hkdf_expand(&seq[i], ciphers.d, c->log) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    /* register cleanup handler once */

    if (peer_secret->ctx) {
        ngx_quic_crypto_cleanup(peer_secret);

    } else {
        cln = ngx_pool_cleanup_add(c->pool, 0);
        if (cln == NULL) {
            return NGX_ERROR;
        }

        cln->handler = ngx_quic_compat_cleanup_encryption_secret;
        cln->data = peer_secret;
    }

    if (ngx_quic_crypto_init(ciphers.c, peer_secret, &key, 1, c->log)
        == NGX_ERROR)
    {
        return NGX_ERROR;
    }

    ngx_explicit_memzero(key.data, key.len);

    return NGX_OK;
}
/* …（同文件无关代码省略）… */
static void
ngx_quic_compat_cleanup_encrypti
