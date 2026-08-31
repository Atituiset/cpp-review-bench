// AUTO-DRAFT from nginx/nginx PR #1662
}
            }

            ngx_free_connection(c);

            c->fd = (ngx_socket_t) -1;
