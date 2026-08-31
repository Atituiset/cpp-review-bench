// AUTO-DRAFT from nginx/nginx PR #1561
{
    off_t                         file_pos;
    u_char                        ch, sep, *pos, *lowcase_key;
    size_t                        size, len, key_len, val_len, padding,  // <<< BUG ANCHOR
                                  allocated;
    ngx_uint_t                    i, n, next, hash, skip_empty, header_params;
    ngx_buf_t                    *b;
    ngx_chain_t                  *cl, *body;
    ngx_http_script_len_code_pt   lcode;

    len = 0;
    header_params = 0;
    ignored = NULL;

                continue;
            }

            len += 1 + key_len + ((val_len > 127) ? 4 : 1) + val_len;
        }
    }

    if (flcf->upstream.pass_request_headers) {

        e.ip = params->values->elts;
        e.pos = b->last;
        e.request = r;
        e.flushed = 1;

                continue;
            }

            *e.pos++ = (u_char) key_len;

            if (val_len > 127) {
            }
            e.ip += sizeof(uintptr_t);

            ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "fastcgi param: \"%*s: %*s\"",
                           key_len, e.pos - (key_len + val_len),
                           val_len, e.pos - val_len);
        }

        b->last = e.pos;
    }
