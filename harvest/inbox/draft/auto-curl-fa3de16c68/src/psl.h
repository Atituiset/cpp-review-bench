// AUTO-DRAFT from curl/curl PR #15289
#ifdef USE_LIBPSL
#include <libpsl.h>

#define PSL_TTL (72 * 3600)     /* PSL time to live before a refresh. */

struct PslCache {
