// AUTO-DRAFT from nginx/nginx PR #1598
return NGX_ERROR;
    }

    if (event == NGX_READ_EVENT) {
        FD_SET(c->fd, &master_read_fd_set);

        return NGX_CONF_OK;
    }

    /* disable warning: the default FD_SETSIZE is 1024U in FreeBSD 5.x */

    if (cycle->connection_n > FD_SETSIZE) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                      "the maximum number of files "
