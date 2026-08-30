// AUTO-DRAFT from curl/curl PR #15289
#include "tool_ipfs.h"
#include "dynbuf.h"
#ifdef DEBUGBUILD
#include "easyif.h"  /* for libcurl's debug-only curl_easy_perform_ev() */
#endif

#include "memdebug.h" /* keep this as LAST include */
