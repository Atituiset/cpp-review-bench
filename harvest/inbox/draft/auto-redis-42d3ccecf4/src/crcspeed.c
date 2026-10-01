// AUTO-DRAFT from redis/redis PR #15532
* reasonable for Intel CPUs made since 2010. Please adjust as necessary if
 * or when your CPU has more load / execute units. We've written benchmark code
 * to help you tune your platform, see crc64Test. */
#if defined(__i386__) || defined(__X86_64__)
static size_t CRC64_TRI_CUTOFF = (2*1024);
static size_t CRC64_DUAL_CUTOFF = (128);
#else
