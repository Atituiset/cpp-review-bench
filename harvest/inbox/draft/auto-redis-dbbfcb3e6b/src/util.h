// AUTO-DRAFT from redis/redis PR #15045
#endif
}

#ifndef static_assert
#define static_assert(expr, lit) extern char __static_assert_failure[(expr) ? 1:-1]
#endif
