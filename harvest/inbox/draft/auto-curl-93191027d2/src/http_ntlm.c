// AUTO-DRAFT from curl/curl PR #1848
#include "vauth/vauth.h"
#include "url.h"

#if defined(USE_NSS)
#include "vtls/nssg.h"
#elif defined(USE_WINDOWS_SSPI)
#include "curl_sspi.h"
  DEBUGASSERT(conn);
  DEBUGASSERT(conn->data);

#ifdef USE_NSS
  if(CURLE_OK != Curl_nss_force_init(conn->data))
    return CURLE_OUT_OF_MEMORY;
#endif
