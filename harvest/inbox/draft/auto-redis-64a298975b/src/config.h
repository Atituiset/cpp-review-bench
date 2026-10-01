// AUTO-DRAFT from redis/redis PR #14770
#if defined (__x86_64__) && ((defined(__GNUC__) && __GNUC__ >= 5) || (defined(__clang__) && __clang_major__ >= 4))
#if defined(__has_attribute) && __has_attribute(target)
#define HAVE_AVX512
#define ATTRIBUTE_TARGET_AVX512_POPCOUNT __attribute__((target("avx512f,avx512vpopcntdq")))
#endif
#endif
