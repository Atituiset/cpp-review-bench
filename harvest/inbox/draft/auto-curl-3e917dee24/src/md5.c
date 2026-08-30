// AUTO-DRAFT from curl/curl PR #15289
typedef struct md5_ctx my_md5_ctx;

static CURLcode my_md5_init(my_md5_ctx *ctx)
{
  md5_init(ctx);
  return CURLE_OK;
}

static void my_md5_update(my_md5_ctx *ctx,
                          const unsigned char *input,
                          unsigned int inputLen)
{
  md5_update(ctx, inputLen, input);
}

static void my_md5_final(unsigned char *digest, my_md5_ctx *ctx)
{
  md5_digest(ctx, 16, digest);
}

typedef MD5_CTX my_md5_ctx;

static CURLcode my_md5_init(my_md5_ctx *ctx)
{
  if(!MD5_Init(ctx))
    return CURLE_OUT_OF_MEMORY;

  return CURLE_OK;
}

static void my_md5_update(my_md5_ctx *ctx,
                          const unsigned char *input,
                          unsigned int len)
{
  (void)MD5_Update(ctx, input, len);
}

static void my_md5_final(unsigned char *digest, my_md5_ctx *ctx)
{
  (void)MD5_Final(digest, ctx);
}

typedef mbedtls_md5_context my_md5_ctx;

static CURLcode my_md5_init(my_md5_ctx *ctx)
{
#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
  if(mbedtls_md5_starts(ctx))
  return CURLE_OK;
}

static void my_md5_update(my_md5_ctx *ctx,
                          const unsigned char *data,
                          unsigned int length)
{
#endif
}

static void my_md5_final(unsigned char *digest, my_md5_ctx *ctx)
{
#if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
  (void) mbedtls_md5_finish(ctx, digest);
   reliable than defining COMMON_DIGEST_FOR_OPENSSL on older cats. */
#  define my_md5_ctx CC_MD5_CTX

static CURLcode my_md5_init(my_md5_ctx *ctx)
{
  if(!CC_MD5_Init(ctx))
    return CURLE_OUT_OF_MEMORY;

  return CURLE_OK;
}

static void my_md5_update(my_md5_ctx *ctx,
                          const unsigned char *input,
                          unsigned int inputLen)
{
  CC_MD5_Update(ctx, input, inputLen);
}

static void my_md5_final(unsigned char *digest, my_md5_ctx *ctx)
{
  CC_MD5_Final(digest, ctx);
}
};
typedef struct md5_ctx my_md5_ctx;

static CURLcode my_md5_init(my_md5_ctx *ctx)
{
  if(!CryptAcquireContext(&ctx->hCryptProv, NULL,
