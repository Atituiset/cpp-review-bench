// AUTO-DRAFT from nginx/nginx PR #1204
#include <ngx_core.h>
  // <<< BUG ANCHOR

#define NGX_PROXY_PROTOCOL_AF_INET          1
#define NGX_PROXY_PROTOCOL_AF_INET6         2


#define ngx_proxy_protocol_parse_uint16(p)                                    \
    ( ((uint16_t) (p)[0] << 8)                                                \
} ngx_proxy_protocol_tlv_entry_t;


static u_char *ngx_proxy_protocol_read_addr(ngx_connection_t *c, u_char *p,
    u_char *last, ngx_str_t *addr);
static u_char *ngx_proxy_protocol_read_port(u_char *p, u_char *last,
    u_char *last);
static ngx_int_t ngx_proxy_protocol_lookup_tlv(ngx_connection_t *c,
    ngx_str_t *tlvs, ngx_uint_t type, ngx_str_t *value);


static ngx_proxy_protocol_tlv_entry_t  ngx_proxy_protocol_tlv_entries[] = {
    { ngx_string("alpn"),       0x01 },
    { ngx_string("authority"),  0x02 },
    { ngx_string("unique_id"),  0x05 },
    { ngx_string("ssl"),        0x20 },
    { ngx_string("netns"),      0x30 },
    { ngx_null_string,          0x00 }
};


static ngx_proxy_protocol_tlv_entry_t  ngx_proxy_protocol_tlv_ssl_entries[] = {
    { ngx_string("version"),    0x21 },
    { ngx_string("cn"),         0x22 },
    { ngx_string("cipher"),     0x23 },
    { ngx_string("sig_alg"),    0x24 },
    { ngx_string("key_alg"),    0x25 },
    { ngx_null_string,          0x00 }
};


u_char *
ngx_proxy_protocol_read(ngx_connection_t *c, u_char *buf, u_char *last)
{
    size_t                 len;
    u_char                *p;
    ngx_proxy_protocol_t  *pp;

    static const u_char signature[] = "\r\n\r\n\0\r\nQUIT\n";

    p = buf;
    len = last - buf;

    if (len >= sizeof(ngx_proxy_protocol_header_t)
        && ngx_memcmp(p, signature, sizeof(signature) - 1) == 0)
    {
        return ngx_proxy_protocol_v2_read(c, buf, last);
    }
    command = header->version_command & 0x0f;

    /* only PROXY is supported */
    if (command != 1) {
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, c->log, 0,
                       "PROXY protocol v2 unsupported command %ui", command);
        return en
