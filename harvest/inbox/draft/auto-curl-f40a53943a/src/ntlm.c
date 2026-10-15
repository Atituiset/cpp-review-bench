// AUTO-DRAFT from curl/curl PR #1848
#include "rand.h"
#include "vtls/vtls.h"

#ifdef USE_NSS
#include "vtls/nssg.h" /* for Curl_nss_force_init() */
#endif

  unsigned char *type2 = NULL;
  size_t type2_len = 0;

#if defined(USE_NSS)
  /* Make sure the crypto backend is initialized */
  result = Curl_nss_force_init(data);
  if(result)
