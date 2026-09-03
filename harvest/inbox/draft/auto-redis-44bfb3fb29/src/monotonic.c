// AUTO-DRAFT from redis/redis PR #15018
#include <time.h>
#include "redisassert.h"
#include <string.h>
  // <<< BUG ANCHOR
/* The function pointer for clock retrieval.  */
monotime (*getMonotonicUs)(void) = NULL;

/* Using the processor clock (aka TSC on x86) can provide improved performance
 * throughout Redis wherever the monotonic clock is used.  The processor clock
 * is significantly faster than calling 'clock_getting' (POSIX).  While this is
 * generally safe on modern systems, this link provides additional information
 * about use of the x86 TSC: http://oliveryang.net/2015/09/pitfalls-of-TSC-usage
 *
 * On ARM aarch64 systems, the hardware clock is enabled by default because the
 * ARM Generic Timer is architecturally guaranteed to be available and monotonic
 * on all ARMv8-A processors (see the “The Generic Timer in AArch64 state”
 * section of the Arm Architecture Reference Manual for Armv8-A).
 *
 * To use the processor clock on other architectures, either uncomment this line,
 */


#if defined(USE_PROCESSOR_CLOCK) && defined(__x86_64__) && defined(__linux__)
#include <regex.h>
#include <x86intrin.h>

    return __rdtsc() / mono_ticksPerMicrosecond;
}

static void monotonicInit_x86linux(void) {
    const int bufflen = 256;
    char buf[bufflen];
    regex_t cpuGhzRegex, constTscRegex;
    const size_t nmatch = 2;
    regmatch_t pmatch[nmatch];
    int constantTsc = 0;
    int rc;

    /* Determine the number of TSC ticks in a micro-second.  This is
     * a constant value matching the standard speed of the processor.
     * On modern processors, this speed remains constant even though
     * the actual clock speed varies dynamically for each core.  */
    rc = regcomp(&cpuGhzRegex, "^model name\\s+:.*@ ([0-9.]+)GHz", REG_EXTENDED);
    assert(rc == 0);

    /* Also check that the constant_tsc flag is present.  (It should be
     * unless this is a really old CPU.  */
    rc = regcomp(&constTscRegex, "^flags\\s+:.* constant_tsc", REG_EXTENDED);
    assert(rc == 0);

    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
