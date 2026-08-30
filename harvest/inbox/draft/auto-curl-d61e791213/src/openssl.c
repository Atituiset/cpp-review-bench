// AUTO-DRAFT from curl/curl PR #3693
return 0;
    }
  // <<< BUG ANCHOR
    file_type = do_file_type(key_type);

    switch(file_type) {
    case SSL_FILETYPE_PEM:
      if(cert_done)
        break;
      if(!key_file)
        /* cert & key can only be in PEM case in the same file */
        key_file = cert_file;
      /* FALLTHROUGH */
    case SSL_FILETYPE_ASN1:
      if(SSL_CTX_use_PrivateKey_file(ctx, key_file, file_type) != 1) {
