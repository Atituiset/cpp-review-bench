// AUTO-DRAFT from curl/curl PR #15289
};
typedef struct ossl_sha256_ctx my_sha256_ctx;

static CURLcode my_sha256_init(my_sha256_ctx *ctx)
{
  ctx->openssl_ctx = EVP_MD_CTX_create();
  if(!ctx->openssl_ctx)
    return CURLE_OUT_OF_MEMORY;
  return CURLE_OK;
}

static void my_sha256_update(my_sha256_ctx *ctx,
                             const unsigned char *data,
                             unsigned int length)
{
  EVP_DigestUpdate(ctx->openssl_ctx, data, length);
}

static void my_sha256_final(unsigned char *digest, my_sha256_ctx *ctx)
{
  EVP_DigestFinal_ex(ctx->openssl_ctx, digest, NULL);
  EVP_MD_CTX_destroy(ctx->openssl_ctx);
}

typedef struct sha256_ctx my_sha256_ctx;

static CURLcode my_sha256_init(my_sha256_ctx *ctx)
{
  sha256_init(ctx);
  return CURLE_OK;
}

static void my_sha256_update(my_sha256_ctx *ctx,
                             const unsigned char *data,
                             unsigned int length)
{
  sha256_update(ctx, length, data);
}

static void my_sha256_final(unsigned char *digest, my_sha256_ctx *ctx)
{
  sha256_digest(ctx, SHA256_DIGEST_SIZE, digest);
}

typedef mbedtls_sha256_context my_sha256_ctx;

static CURLcode my_sha256_init(my_sha256_ctx *ctx)
{
#if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
  (void) mbedtls_sha256_starts(ctx, 0);
  return CURLE_OK;
}

static void my_sha256_update(my_sha256_ctx *ctx,
                             const unsigned char *data,
                             unsigned int length)
{
#endif
}

static void my_sha256_final(unsigned char *digest, my_sha256_ctx *ctx)
{
#if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
  (void) mbedtls_sha256_finish(ctx, digest);
#elif defined(AN_APPLE_OS)
typedef CC_SHA256_CTX my_sha256_ctx;

static CURLcode my_sha256_init(my_sha256_ctx *ctx)
{
  (void) CC_SHA256_Init(ctx);
  return CURLE_OK;
}

static void my_sha256_update(my_sha256_ctx *ctx,
                             const unsigned char *data,
                             unsigned int length)
{
  (void) CC_SHA256_Update(ctx, data, length);
}

stati
