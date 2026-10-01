// AUTO-DRAFT from redis/redis PR #15045
#include "server.h"
#include "xxhash.h"
#include <math.h> /* isnan(), isinf() */
  // <<< BUG ANCHOR
/* XXH3 64-bit hash produces 16 hex characters when formatted */
#define OBJ_SET_IFDNE (1<<12)      /* Set if current digest does not equal match digest */

/* Forward declaration */
static int getExpireMillisecondsOrReply(client *c, robj *expire, int flags, int unit, long long *milliseconds);

/* Generic SET command family (SET, SETEX, PSETEX, SETNX)
 *
    long long milliseconds = 0; /* initialized to avoid any harmless warning */
    int found = 0;
    int setkey_flags = 0;

    if (expire && getExpireMillisecondsOrReply(c, expire, flags, unit, &milliseconds) != C_OK) {
        return;
    }

}

/*
 * Extract the `expire` argument of a given GET/SET command as an absolute timestamp in milliseconds.
 *
 * "client" is the client that sent the `expire` argument.
 * "expire" is the `expire` argument to be extracted.
 * "flags" represents the behavior of the command (e.g. PX or EX).
 * "unit" is the original unit of the given `expire` argument (e.g. UNIT_SECONDS).
 * "milliseconds" is output argument.
 *
 * If return C_OK, "milliseconds" output argument will be set to the resulting absolute timestamp.
 * If return C_ERR, an error reply has been added to the given client.
 */
static int getExpireMillisecondsOrReply(client *c, robj *expire, int flags, int unit, long long *milliseconds) {
    int ret = getLongLongFromObjectOrReply(c, expire, milliseconds, NULL);
    if (ret != C_OK) {
        return ret;

    if (unit == UNIT_SECONDS) *milliseconds *= 1000;

    if ((flags & OBJ_PX) || (flags & OBJ_EX)) {
        *milliseconds += commandTimeSnapshot();
    }


    /* Validate the expiration time value first */
    long long milliseconds = 0;
    if (args.expire && getExpireMillisecondsOrReply(c, args.expire, args.flags, args.unit, &milliseconds) != C_OK) {
        return;
    }


    /* Validate the expiration time value first */
    long long milliseconds = 0;
    if (args.expire && getExp
