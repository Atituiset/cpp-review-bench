// AUTO-DRAFT from nginx/nginx PR #413
ngx_quic_close_streams(ngx_connection_t *c, ngx_quic_connection_t *qc)
{
    ngx_pool_t         *pool;
    ngx_queue_t        *q;
    ngx_rbtree_t       *tree;
    ngx_connection_t   *sc;
    ngx_rbtree_node_t  *node;
        return NGX_OK;
    }
  // <<< BUG ANCHOR
    node = ngx_rbtree_min(tree->root, tree->sentinel);

    while (node) {
        }

        sc->read->error = 1;
        sc->write->error = 1;

        ngx_quic_set_event(sc->read);
        ngx_quic_set_event(sc->write);

        sc->close = 1;
        sc->read->handler(sc->read);
    }

    if (tree->root == tree->sentinel) {
        return NGX_OK;
    }
