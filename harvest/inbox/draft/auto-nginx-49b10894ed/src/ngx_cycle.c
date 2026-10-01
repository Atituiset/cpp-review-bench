// AUTO-DRAFT from nginx/nginx PR #931
== NGX_OK)
                {
                    nls[n].fd = ls[i].fd;
                    nls[n].inherited = ls[i].inherited;  // <<< BUG ANCHOR
                    nls[n].previous = &ls[i];
                    ls[i].remain = 1;

                    if (ls[i].backlog != nls[n].backlog) {
                        nls[n].listen = 1;
            }

            if (nls[n].fd == (ngx_socket_t) -1) {
                nls[n].open = 1;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
                if (nls[n].accept_filter) {
                    nls[n].add_deferred = 1;
        }

    } else {
        ls = cycle->listening.elts;
        for (i = 0; i < cycle->listening.nelts; i++) {
            ls[i].open = 1;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
            if (ls[i].accept_filter) {
                ls[i].add_deferred = 1;
            }
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
            if (ls[i].deferred_accept) {
                ls[i].add_deferred = 1;
            }
#endif
        }
    }

    if (ngx_open_listening_sockets(cycle) != NGX_OK) {
