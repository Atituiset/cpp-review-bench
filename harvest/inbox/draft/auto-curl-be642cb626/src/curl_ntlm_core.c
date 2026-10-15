// AUTO-DRAFT from curl/curl PR #1848
* https://www.innovation.ch/java/ntlm.html
 */
  // <<< BUG ANCHOR
#if !defined(USE_WINDOWS_SSPI) || defined(USE_WIN32_CRYPTO)

#ifdef USE_OPENSSL
#  define MD5_DIGEST_LENGTH 16
#  define MD4_DIGEST_LENGTH 16

#elif defined(USE_MBEDTLS)

#  include <mbedtls/des.h>
#  include <mbedtls/md4.h>
#  if !defined(MBEDTLS_MD4_C)
#    include "curl_md4.h"
#  endif

#elif defined(USE_NSS)

#  include <nss.h>
#  include "curl_md4.h"
#  define MD5_DIGEST_LENGTH MD5_LENGTH

#elif defined(USE_DARWINSSL)

#  include <CommonCrypto/CommonCryptor.h>
  gcry_cipher_setkey(*des, key, sizeof(key));
}

#elif defined(USE_MBEDTLS)

static bool encrypt_des(const unsigned char *in, unsigned char *out,
                        const unsigned char *key_56)
{
  mbedtls_des_context ctx;
  char key[8];

  /* Expand the 56-bit key to 64-bits */
  extend_key_56_to_64(key_56, key);

  /* Set the key parity to odd */
  mbedtls_des_key_set_parity((unsigned char *) key);

  /* Perform the encryption */
  mbedtls_des_init(&ctx);
  mbedtls_des_setkey_enc(&ctx, (unsigned char *) key);
  return mbedtls_des_crypt_ecb(&ctx, in, out) == 0;
}

#elif defined(USE_NSS)

/*
  return rv;
}

#elif defined(USE_DARWINSSL)

static bool encrypt_des(const unsigned char *in, unsigned char *out,
  setup_des_key(keys + 14, &des);
  gcry_cipher_encrypt(des, results + 16, 8, plaintext, 8);
  gcry_cipher_close(des);
#elif defined(USE_MBEDTLS) || defined(USE_NSS) || defined(USE_DARWINSSL) \
  || defined(USE_OS400CRYPTO) || defined(USE_WIN32_CRYPTO)
  encrypt_des(plaintext, results, keys);
  encrypt_des(plaintext, results + 8, keys + 7);
    setup_des_key(pw + 7, &des);
    gcry_cipher_encrypt(des, lmbuffer + 8, 8, magic, 8);
    gcry_cipher_close(des);
#elif defined(USE_MBEDTLS) || defined(USE_NSS) || defined(USE_DARWINSSL) \
  || defined(USE_OS400CRYPTO) || defined(USE_WIN32_CRYPTO)
    encrypt_des(magic, lmbuffer, pw);
    encrypt_des(magic, lmbuffer + 8, pw + 7);
    gcry_md_write(MD4pw, pw, 2 * len);
    memcpy(ntbuffer, gcry_md_read(MD4pw, 0), 
