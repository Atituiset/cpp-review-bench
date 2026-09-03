// AUTO-DRAFT from curl/curl PR #7808
in NTLM type-3 messages.
 */

#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)

#ifdef USE_WOLFSSL
#include <wolfssl/options.h>
#elif defined(USE_WIN32_CRYPTO)
#  include <wincrypt.h>
#else
#  error "Can't compile NTLM support without a crypto library."
#endif

#include "urldata.h"
  key[7] = (unsigned char) ((key_56[6] << 1) & 0xFF);
}

#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)
/*
 * Turns a 56 bit key into the 64 bit, odd parity key and sets the key.  The
 * key schedule ks is also set.
                            const unsigned char *plaintext,
                            unsigned char *results)
{
#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)
  DES_key_schedule ks;

  setup_des_key(keys, DESKEY(ks));
  {
    /* Create LanManager hashed password. */

#if defined(USE_OPENSSL) || defined(USE_WOLFSSL)
    DES_key_schedule ks;

    setup_des_key(pw, DESKEY(ks));
