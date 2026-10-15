// AUTO-DRAFT from redis/redis PR #15392
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
        if (count == 0) {
            return RedisModule_ReplyWithEmptyArray(ctx);
        }
    }

    /* Open key. */
