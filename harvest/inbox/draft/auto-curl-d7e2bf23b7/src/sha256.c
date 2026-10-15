// AUTO-DRAFT from curl/curl PR #7808
#include <openssl/opensslv.h>
  // <<< BUG ANCHOR
#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
#define USE_OPENSSL_SHA256
#endif

#if defined(USE_OPENSSL_SHA256)

/* When OpenSSL is available we use the SHA256-function from OpenSSL */
#include <openssl/sha.h>

#elif defined(USE_GNUTLS)

/* The last #include file should be: */
#include "memdebug.h"

typedef struct sha256_ctx SHA256_CTX;

static void SHA256_Init(SHA256_CTX *ctx)
{
  sha256_init(ctx);
}

static void SHA256_Update(SHA256_CTX *ctx,
                          const unsigned char *data,
                          unsigned int length)
{
  sha256_update(ctx, length, data);
}

static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
{
  sha256_digest(ctx, SHA256_DIGEST_SIZE, digest);
}
/* The last #include file should be: */
#include "memdebug.h"

typedef mbedtls_sha256_context SHA256_CTX;

static void SHA256_Init(SHA256_CTX *ctx)
{
#if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
  (void) mbedtls_sha256_starts(ctx, 0);
#endif
}

static void SHA256_Update(SHA256_CTX *ctx,
                          const unsigned char *data,
                          unsigned int length)
{
#if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
  (void) mbedtls_sha256_update(ctx, data, length);
#endif
}

static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
{
#if !defined(HAS_MBEDTLS_RESULT_CODE_BASED_FUNCTIONS)
  (void) mbedtls_sha256_finish(ctx, digest);
/* The last #include file should be: */
#include "memdebug.h"

typedef CC_SHA256_CTX SHA256_CTX;

static void SHA256_Init(SHA256_CTX *ctx)
{
  (void) CC_SHA256_Init(ctx);
}

static void SHA256_Update(SHA256_CTX *ctx,
                          const unsigned char *data,
                          unsigned int length)
{
  (void) CC_SHA256_Update(ctx, data, length);
}

static void SHA256_Final(unsigned char *digest, SHA256_CTX *ctx)
{
  (void) CC_SHA256_Final(digest, ctx);
}
  HCRYPTPROV hCryptProv;
  HCRYPTHASH hHash;
};
typedef struct sha256_ctx SHA256_CTX;

#if !defined(CALG_
