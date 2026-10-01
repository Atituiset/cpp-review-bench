// AUTO-DRAFT from nginx/nginx PR #634
#include <openssl/bn.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/dh.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif
#endif


typedef struct ngx_ssl_ocsp_s   ngx_ssl_ocsp_t;
