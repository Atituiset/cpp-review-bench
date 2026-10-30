// AUTO-DRAFT from curl/curl PR #23686
      *done = TRUE;
      goto out;
    }
    if(SSL_get_error(octx->ssl, rc) == SSL_ERROR_WANT_WRITE) {  // <<< BUG ANCHOR
      CURL_TRC_CF(data, cf, "SSL shutdown still wants to send");
      connssl->io_need = CURL_SSL_IO_NEED_SEND;
      goto out;
    }
    /* Having sent the close notify, we use SSL_read() to get the
     * missing close notify from the server. */
