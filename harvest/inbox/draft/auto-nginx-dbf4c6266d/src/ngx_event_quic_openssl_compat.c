// AUTO-DRAFT from nginx/nginx PR #348
wbio = BIO_new(BIO_s_null());
    if (wbio == NULL) {
        return 0;
    }
