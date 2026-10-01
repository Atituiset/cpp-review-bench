// AUTO-DRAFT from redis/redis PR #15111
1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
};
  // <<< BUG ANCHOR
/* Maximum mantissa for fast path: 2^53 */
#define MAX_MANTISSA_FAST_PATH 9007199254740992ULL  /* 2^53 */

    return (uint32_t)val;
}

/* Parse a decimal number string into components.
 * This follows the fast_float algorithm closely. */
static inline int parse_number_string(const char *p, const char *pend, double *result, const char **endptr) {
        if (digit_count > MAX_DIGITS) return 0;
    }

    /* Check if we're within fast path bounds */
    if (exponent < MIN_EXPONENT_FAST_PATH) return 0;
    if (exponent > MAX_EXPONENT_FAST_PATH) return 0;

    double value;
    if (mantissa <= MAX_MANTISSA_FAST_PATH) {
        /* Clinger fast path: all operands exact in double precision,
         * single multiply/divide produces a correctly-rounded result. */
        value = (double)mantissa;
        if (exponent < 0)       value = value / powers_of_ten[-exponent];
        else if (exponent > 0)  value = value * powers_of_ten[exponent];
    } else {
#ifdef __SIZEOF_INT128__
        /* Widened fast path for 17-19 significant-digit mantissas.
         *
         * (double)mantissa alone loses up to 11 bits when mantissa > 2^53,
         * so the existing Clinger path would yield up to 1 ULP vs strtod.
         * We recover full precision by doing the multiply/divide in 128-bit
         * integer arithmetic (correctly-rounded by construction). Cases
         * outside the supported exponent range fall through to strtod.
         *
         * Requires __uint128_t (GCC/Clang builtin, available on every 64-bit
         * target Redis supports). 32-bit builds take the strtod() fallback. */
        if (exponent < -19 || exponent > 19) return 0;

        if (exponent >= 0) {
            /* (mantissa * 10^e) fits in 128 bits. Convert exactly: the
             * single (double) cast from __uint128_t rounds to nearest. */
            __uint128_t prod = (__uint128_t)mantissa * (uint64_t)powers_of_ten[exponent];
 
