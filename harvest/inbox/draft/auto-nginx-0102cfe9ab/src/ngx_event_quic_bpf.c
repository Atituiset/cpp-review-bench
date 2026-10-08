// AUTO-DRAFT from nginx/nginx PR #1756
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_QUIC_BPF_VARNAME  "NGINX_BPF_MAPS"
#define NGX_QUIC_BPF_VARSEP    ';'
#define NGX_QUIC_BPF_ADDRSEP   '#'
/* …（同文件无关代码省略）… */
#define ngx_quic_bpf_get_conf(cycle)                                          \
    (ngx_quic_bpf_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_quic_bpf_module)
/* …（同文件无关代码省略）… */
#define ngx_core_get_conf(cycle)                                              \
    (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module)


typedef struct {
    ngx_queue_t           queue;
    int                   map_fd;

    struct sockaddr      *sockaddr;
    socklen_t             socklen;
    ngx_uint_t            unused;     /* unsigned  unused:1; */
} ngx_quic_sock_group_t;


typedef struct {
    ngx_flag_t            enabled;
    ngx_uint_t            map_size;
    ngx_queue_t           groups;     /* of ngx_quic_sock_group_t */
} ngx_quic_bpf_conf_t;


static void *ngx_quic_bpf_create_conf(ngx_cycle_t *cycle);
static ngx_int_t ngx_quic_bpf_module_init(ngx_cycle_t *cycle);

static void ngx_quic_bpf_cleanup(void *data);
static ngx_inline void ngx_quic_bpf_close(ngx_log_t *log, int fd,
    const char *name);

static ngx_quic_sock_group_t *ngx_quic_bpf_find_group(ngx_quic_bpf_conf_t *bcf,
    ngx_listening_t *ls);
static ngx_quic_sock_group_t *ngx_quic_bpf_alloc_group(ngx_cycle_t *cycle,
    struct sockaddr *sa, socklen_t socklen);
static ngx_quic_sock_group_t *ngx_quic_bpf_create_group(ngx_cycle_t *cycle,
    ngx_listening_t *ls);
static ngx_quic_sock_group_t *ngx_quic_bpf_get_group(ngx_cycle_t *cycle,
    ngx_listening_t *ls);
static ngx_int_t ngx_quic_bpf_group_add_socket(ngx_cycle_t *cycle,
    ngx_listening_t *ls);
static uint64_t ngx_quic_bpf_socket_key(ngx_fd_t fd, ngx_log_t *log);

static ngx_int_t ngx_quic_bpf_export_maps(ngx_cycle_t *cycle);
static ngx_int_t ngx_quic_bpf_import_maps(ngx_cycle_t *cycle);
/* …（同文件无关代码省略）… */
static ngx_core_module_t  ngx_quic_bpf_module_ctx = {
    ngx_string("quic_bpf"),
    ngx_quic_bpf_create_conf,
    NULL
};


/* …（同文件无关代码省略）… */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static void *
ngx_quic_bpf_create_conf(ngx_cycle_t *cycle)
{
    ngx_quic_bpf_conf_t  *bcf;

    bcf = ngx_pcalloc(cycle->pool, sizeof(ngx_quic_bpf_conf_t));
/* …（同文件无关代码省略）… */
    bcf->enabled = NGX_CONF_UNSET;
    bcf->map_size = NGX_CONF_UNSET_UINT;

    ngx_queue_init(&bcf->groups);

    return bcf;
}


static ngx_int_t
ngx_quic_bpf_module_init(ngx_cycle_t *cycle)
{
    ngx_uint_t            i;
    ngx_listening_t      *ls;
    ngx_core_conf_t      *ccf;
    ngx_pool_cleanup_t   *cln;
    ngx_quic_bpf_conf_t  *bcf;

    if (ngx_test_config) {
        /*
/* …（同文件无关代码省略）… */
    ccf = ngx_core_get_conf(cycle);
    bcf = ngx_quic_bpf_get_conf(cycle);

    ngx_conf_init_value(bcf->enabled, 0);

    bcf->map_size = ccf->worker_processes * 4;

    cln = ngx_pool_cleanup_add(cycle->pool, 0);
    if (cln == NULL) {
/* …（同文件无关代码省略）… */
        }
    }

    if (ngx_quic_bpf_export_maps(cycle) != NGX_OK) {
        goto failed;
    }
/* …（同文件无关代码省略）… */
}


static void
ngx_quic_bpf_cleanup(void *data)
{
    ngx_quic_bpf_conf_t  *bcf = (ngx_quic_bpf_conf_t *) data;

    ngx_queue_t            *q;
    ngx_quic_sock_group_t  *grp;

    for (q = ngx_queue_head(&bcf->groups);
         q != ngx_queue_sentinel(&bcf->groups);
         q = ngx_queue_next(q))
    {
        grp = ngx_queue_data(q, ngx_quic_sock_group_t, queue);

        ngx_quic_bpf_close(ngx_cycle->log, grp->map_fd, "map");
    }
}


static ngx_inline void
ngx_quic_bpf_close(ngx_log_t *log, int fd, const char *name)
{
    if (close(fd) != -1) {
        return;
    }
/* …（同文件无关代码省略）… */
}


static ngx_quic_sock_group_t *
ngx_quic_bpf_find_group(ngx_quic_bpf_conf_t *bcf, ngx_listening_t *ls)
{
/* …（同文件无关代码省略）… */


static ngx_quic_sock_group_t *
ngx_quic_bpf_alloc_group(ngx_cycle_t *cycle, struct sockaddr *sa,
    socklen_t socklen)
{
    ngx_quic_bpf_conf_t    *bcf;
    ngx_quic_sock_group_t  *grp;
/* …（同文件无关代码省略）… */
        return NULL;
    }

    grp->socklen = socklen;
    grp->sockaddr = ngx_palloc(cycle->pool, socklen);
    if (grp->sockaddr == NULL) {
        return NULL;
    }
    ngx_memcpy(grp->sockaddr, sa, socklen);

    ngx_queue_insert_tail(&bcf->groups, &grp->queue);

/* …（同文件无关代码省略）… */
static ngx_quic_sock_group_t *
ngx_quic_bpf_create_group(ngx_cycle_t *cycle, ngx_listening_t *ls)
{
    int                     progfd, failed, flags, rc;
    ngx_quic_bpf_conf_t    *bcf;
    ngx_quic_sock_group_t  *grp;

    bcf = ngx_quic_bpf_get_conf(cycle);

    if (!bcf->enabled) {
        return NULL;
    }

    grp = ngx_quic_bpf_alloc_group(cycle, ls->sockaddr, ls->socklen);
    if (grp == NULL) {
        return NULL;
    }

    grp->map_fd = ngx_bpf_map_create(cycle->log, BPF_MAP_TYPE_SOCKHASH,
                                     sizeof(uint64_t), sizeof(uint64_t),
                                     bcf->map_size, 0);
    if (grp->map_fd == -1) {
        goto failed;
    }

    flags = fcntl(grp->map_fd, F_GETFD);
    if (flags == -1) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, errno,
                      "quic bpf getfd failed");
        goto failed;
    }

    /* need to inherit map during binary upgrade after exec */
    flags &= ~FD_CLOEXEC;

    rc = fcntl(grp->map_fd, F_SETFD, flags);
    if (rc == -1) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, errno,
                      "quic bpf setfd failed");
        goto failed;
    }

    ngx_bpf_program_link(&ngx_quic_reuseport_helper,
                         "ngx_quic_sockmap", grp->map_fd);

    progfd = ngx_bpf_load_program(cycle->l
