// AUTO-DRAFT from redis/redis PR #15492
return 1;
}

static int applyTlsCfg(const char **err) {
    /* If TLS is enabled, try to configure OpenSSL. */
    if (server.tls_port || server.tls_replication || server.tls_cluster) {
    createStringConfig("tls-protocols", NULL, MODIFIABLE_CONFIG, EMPTY_STRING_IS_NULL, server.tls_ctx_config.protocols, NULL, NULL, applyTlsCfg),
    createStringConfig("tls-ciphers", NULL, MODIFIABLE_CONFIG, EMPTY_STRING_IS_NULL, server.tls_ctx_config.ciphers, NULL, NULL, applyTlsCfg),
    createStringConfig("tls-ciphersuites", NULL, MODIFIABLE_CONFIG, EMPTY_STRING_IS_NULL, server.tls_ctx_config.ciphersuites, NULL, NULL, applyTlsCfg),

    /* Special configs */
    createSpecialConfig("dir", NULL, MODIFIABLE_CONFIG | PROTECTED_CONFIG | DENY_LOADING_CONFIG, setConfigDirOption, getConfigDirOption, rewriteConfigDirOption, NULL),
