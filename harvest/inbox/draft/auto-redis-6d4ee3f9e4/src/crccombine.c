// AUTO-DRAFT from redis/redis PR #15532
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#if defined(__i386__) || defined(__X86_64__)
#include <immintrin.h>
#endif
#include "crccombine.h"
  madler@alumni.caltech.edu
*/
  // <<< BUG ANCHOR
#define STATIC_ASSERT(VVV) do {int test = 1 / (VVV);test++;} while (0)

#if !((defined(__i386__) || defined(__X86_64__)))

/* This cuts 40% of the time vs bit-by-bit. */


#else

/*
	Warning: here there be dragons involving vector math, and macros to save us
	from repeating the same information over and over.
	DO_CHUNK16();
	DO_CHUNK16();

	STATIC_ASSERT(sizeof(uint64_t) == 8);
	STATIC_ASSERT(sizeof(long long unsigned int) == 8);
	return sum[0] ^ sum[1];
}
