// AUTO-DRAFT from curl/curl PR #1379
#elif defined(HAVE_POLL_H)
#include <poll.h>
#endif

#define ENABLE_CURLX_PRINTF
/* make the curlx header define all printf() functions to use the curlx_*
#define EINVAL  22 /* errno.h value */
#endif

#if defined(ENABLE_IPV6) && defined(__MINGW32__)
const struct in6_addr in6addr_any = {{ IN6ADDR_ANY_INIT }};
#endif

/* This function returns a pointer to STATIC memory. It converts the given
 * binary lump to a hex formatted string usable for output in logs or
