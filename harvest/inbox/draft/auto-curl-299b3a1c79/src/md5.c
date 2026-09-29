// AUTO-DRAFT from curl/curl PR #7808
#endif
#endif /* USE_MBEDTLS */

#if defined(USE_GNUTLS)

#include <nettle/md5.h>
  md5_digest(ctx, 16, digest);
}

#elif (defined(USE_OPENSSL) && !defined(USE_AMISSL)) || defined(USE_WOLFSSL)

#ifdef USE_WOLFSSL
#include <wolfssl/options.h>
#endif

#if defined(USE_OPENSSL) || (defined(USE_WOLFSSL) && !defined(NO_MD5))
/* When OpenSSL or wolfSSL is available, we use their MD5 functions. */
#include <openssl/md5.h>
#include "curl_memory.h"
/* The last #include file should be: */
#include "memdebug.h"
#endif

#elif defined(USE_MBEDTLS)
