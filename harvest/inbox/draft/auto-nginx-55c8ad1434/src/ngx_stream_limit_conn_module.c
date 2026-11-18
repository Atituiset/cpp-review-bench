// AUTO-DRAFT from nginx/nginx PR #1869
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    u_char                          color;
    u_char                          len;
    u_short                         conn;
    u_char                          data[1];
} ngx_stream_limit_conn_node_t;
/* …（同文件无关代码省略）… */
typedef struct {
    ngx_shm_zone_t                 *shm_zone;
    ngx_rbtree_node_t              *node;
} ngx_stream_limit_conn_cleanup_t;
/* …（同文件无关代码省略）… */
typedef struct {
    ngx_rbtree_t                    rbtree;
    ngx_rbtree_node_t               sentinel;
} ngx_stream_limit_conn_shctx_t;
/* …（同文件无关代码省略）… */
typedef struct {
    ngx_stream_limit_conn_shctx_t  *sh;
    ngx_slab_pool_t                *shpool;
    ngx_stream_complex_value_t      key;
} ngx_stream_limit_conn_ctx_t;
/* …（同文件无关代码省略）… */
    ngx_str_t *key, uint32_t hash);
static void ngx_stream_limit_conn_cleanup(void *data);
static ngx_inline void ngx_stream_limit_conn_cleanup_all(ngx_pool_t *pool);

static ngx_int_t ngx_stream_limit_conn_status_variable(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);
/* …（同文件无关代码省略）… */
        cln = ngx_pool_cleanup_add(s->connection->pool,
                                   sizeof(ngx_stream_limit_conn_cleanup_t));
        if (cln == NULL) {
            return NGX_ERROR;
        }

/* …（同文件无关代码省略）… */
static void
ngx_stream_limit_conn_cleanup(void *data)
{
    ngx_stream_limit_conn_cleanup_t  *lccln = data;

    ngx_rbtree_node_t             *node;
    ngx_stream_limit_conn_ctx_t   *ctx;
    ngx_stream_limit_conn_node_t  *lc;

    ctx = lccln->shm_zone->data;
    node = lccln->node;
    lc = (ngx_stream_limit_conn_node_t *) &node->color;

    ngx_shmtx_lock(&ctx->shpool->mutex);

    ngx_log_debug2(NGX_LOG_DEBUG_STREAM, lccln->shm_zone->shm.log, 0,
                   "limit conn cleanup: %08Xi %d", node->key, lc->conn);

    lc->conn--;

    if (lc->conn == 0) {
        ngx_rbtree_delete(&ctx->sh->rbtree, node);
        ngx_slab_free_locked(ctx->shpool, node);
    }

    ngx_shmtx_unlock(&ctx->shpool->mutex);
}
/* …（同文件无关代码省略）… */
static ngx_inline void
ngx_stream_limit_conn_cleanup_all(ngx_pool_t *pool)
{
    ngx_pool_cleanup_t  *cln;

    cln = pool->cleanup;

    while (cln && cln->handler == ngx_stream_limit_conn_cleanup) {
        ngx_stream_limit_conn_cleanup(cln->data);
        cln = cln->next;
    }

    pool->cleanup = cln;
}


static ngx_int_t
ngx_stream_limit_conn_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
/* …（同文件无关代码省略）… */
static ngx_int_t
ngx_stream_limit_conn_status_variable(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    if (s->limit_conn_status == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = ngx_stream_limit_conn_status[s->limit_conn_status - 1].len;
    v->data = ngx_stream_limit_conn_status[s->limit_conn_status - 1].data;

    return NGX_OK;
}
