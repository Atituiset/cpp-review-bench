// AUTO-DRAFT from redis/redis PR #14721
if (!rsc) return;
    if (rsc->ssl) {
        SSL_free(rsc->ssl);
        rsc->ssl = NULL;
    }
