// AUTO-DRAFT from nginx/nginx PR #1756
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
  // <<< BUG ANCHOR
#define debugmsg(fmt, ...)                                                    \
do {                                                                          \
    char __buf[] = fmt;                                                       \
    bpf_trace_printk(__buf, sizeof(__buf), ##__VA_ARGS__);                    \
} while (0)
/* …（同文件无关代码省略）… */
#define NGX_QUIC_PKT_LONG        0x80  /* header form */
#define NGX_QUIC_SERVER_CID_LEN  20


#define advance_data(nbytes)                                                  \
    offset += nbytes;                                                         \
    if (start + offset > end) {                                               \
        debugmsg("cannot read %ld bytes at offset %ld", nbytes, offset);      \
        goto failed;                                                          \
    }                                                                         \
    data = start + offset - 1;


#define ngx_quic_parse_uint64(p)                                              \
    (((__u64)(p)[0] << 56) |                                                  \
     ((__u64)(p)[1] << 48) |                                                  \
/* …（同文件无关代码省略）… */
     ((__u64)(p)[6] << 8)  |                                                  \
     ((__u64)(p)[7]))

/*
 * actual map object is created by the "bpf" system call,
 * all pointers to this variable are replaced by the bpf loader
 */
extern int ngx_quic_sockmap;


SEC(PROGNAME)
int ngx_quic_select_socket_by_dcid(struct sk_reuseport_md *ctx)
{
    int             rc;
    __u64           key;
    size_t          len, offset;
    unsigned char  *start, *end, *data, *dcid;

    start = ctx->data;
    end = (unsigned char *) ctx->data_end;
    offset = 0;

    advance_data(sizeof(struct udphdr)); /* data at UDP header */
    advance_data(1); /* data at QUIC flags */

    if (data[0] & NGX_QUIC_PKT_LONG) {

        advance_data(4); /* data at QUIC version */
        advance_data(1); /* data at DCID len */

        len = data[0];   /* read DCID length */

        if (len < 8) {
            /* it's useless to search for key in such short DCID */
            return SK_PASS;
        }

    } else {
        len = NGX_QUIC_SERVER_CID_LEN;
    }

    dcid = &data[1];
    advance_data(len); /* we expect the packet to have full DCID */

    /* make verifier happy */
    if (dcid + sizeof(__u64) > end) {
        goto failed;
    }

    key = ngx_quic_parse_uint64(dcid);

    rc = bpf_sk_select_reuseport(ctx, &ngx_quic_sockmap, &key, 0);

    switch (rc) {
    case 0:
        debugmsg("nginx quic socket selected by key 0x%llx", key);
        return SK_PASS;

    /* kernel returns positive error numbers, errno.h defines positive */
    case -ENOENT:
        debugmsg("nginx quic default route for key 0x%llx", key);
        /* let the default reuseport logic decide which socket to choose */
        return SK_PASS;

    default:
        debugmsg("nginx quic bpf_sk_select_reuseport err: %d key 0x%llx",
                 rc, key);
        goto failed;
    }

failed:
    /*
     * SK_DROP will generate ICMP, but we may want to process "invalid" packet
     * in userspace quic to investigate further and finally react properly
     * (maybe ignore, maybe send something in response or close connection)
     */
    return SK_PASS;
}
