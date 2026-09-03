// AUTO-DRAFT from nginx/nginx PR #1574
if (b->last_buf) {
            ctx->last = p;
            return NGX_OK;
        }
    }
