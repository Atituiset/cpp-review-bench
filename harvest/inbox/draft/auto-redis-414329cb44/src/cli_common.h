// AUTO-DRAFT from redis/redis PR #15556
#include <hiredis.h>
#include <sdscompat.h> /* Use hiredis' sds compat header that maps sds calls to their hi_ variants */

typedef struct cliSSLconfig {
    /* Requested SNI, or NULL */
    char *sni;
    char* ciphers;
    /* Preferred ciphersuites list, or NULL (applies only to TLSv1.3) */
    char* ciphersuites;
} cliSSLconfig;
