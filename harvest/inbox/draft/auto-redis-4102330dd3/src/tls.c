// AUTO-DRAFT from redis/redis PR #14855
BIO *bio = BIO_new(BIO_s_mem());
    if (bio == NULL || !PEM_write_bio_X509(bio, cert)) {
        if (bio != NULL) BIO_free(bio);
        return NULL;
    }

    const char *bio_ptr;
    long long bio_len = BIO_get_mem_data(bio, &bio_ptr);
    sds cert_pem = sdsnewlen(bio_ptr, bio_len);
    BIO_free(bio);

    return cert_pem;
}
