// AUTO-DRAFT from redis/redis PR #15710
static char monotonic_info_string[32];
  // <<< BUG ANCHOR

/* Using the processor clock (aka TSC on x86) can provide improved performance
 * throughout Redis wherever the monotonic clock is used.  The processor clock
    FILE *cs = fopen("/sys/devices/system/clocksource/clocksource0/current_clocksource", "r");
    if (cs == NULL || fgets(buf, bufflen, cs) == NULL || strncmp(buf, "tsc", 3) != 0) {
        if (cs) fclose(cs);
        fprintf(stderr, "monotonic: x86 linux, kernel clocksource is not 'tsc'\n");
        return;
    }
    fclose(cs);
    regfree(&constTscRegex);

    if (!constantTsc) {
        fprintf(stderr, "monotonic: x86 linux, 'constant_tsc' flag not present\n");
        return;
    }

        if (measured > 0 && labs(measured - nominal_model) * 1000 <= nominal_model) { /* within 0.1% */
            mono_ticksPerMicrosecond = nominal_model;
        } else {
            fprintf(stderr, "monotonic: x86 linux, advertised clock rate "
                    "(%ld ticks/us) unconfirmed by the measured rate "
                    "(%ld ticks/us), using calibration\n",
                    nominal_model, measured);
        }
    }
    }

    if (mono_ticksPerMicrosecond == 0) {
        fprintf(stderr, "monotonic: x86 linux, unable to determine clock rate\n");
        return;
    }

static void monotonicInit_aarch64(void) {
    mono_ticksPerMicrosecond = (long)cntfrq_hz() / 1000L / 1000L;
    if (mono_ticksPerMicrosecond == 0) {
        fprintf(stderr, "monotonic: aarch64, unable to determine clock rate\n");
        return;
    }

static void monotonicInit_riscv(void) {
    mono_ticksPerMicrosecond = (long)get_timebase_frequency() / 1000L / 1000L;
    if (mono_ticksPerMicrosecond == 0) {
        fprintf(stderr, "monotonic: riscv, unable to determine clock rate\n");
        return;
    }
    snprintf(monotonic_info_string, sizeof(monotonic_info_string),



const char * monotonicInit(void) {
    #if defined(__x86_64__) && defined(__linux__)
    if (getMonotonicUs == NULL) monotonicI
