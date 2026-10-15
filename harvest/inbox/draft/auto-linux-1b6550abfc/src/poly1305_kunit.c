// AUTO-DRAFT from torvalds/linux PR #0fcb05b72344f40806cbe810de8872f1698bf053
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <string.h>

static void poly1305(const u8 key[POLY1305_KEY_SIZE], const u8 *data,
		     size_t len, u8 out[POLY1305_DIGEST_SIZE])
{
	struct poly1305_desc_ctx ctx;

	poly1305_init(&ctx, key);
	poly1305_update(&ctx, data, len);
	poly1305_final(&ctx, out);
}
/* …（同文件无关代码省略）… */
 * - Using an all-one-bits r_key tests the key clamping.
 * - Using an all-one-bits s_key tests carries in implementations of the
 *   addition mod 2**128 during finalization.
 * - Using all-one-bits message, and to a lesser extent r_key, tends to maximize
 *   any intermediate accumulator values.  This increases the chance of
 *   detecting bugs that occur only in rare cases where the accumulator values
 *   get very large, for example the bug fixed by commit 678cce4019d746da
 *   ("crypto: x86/poly1305 - fix overflow during partial reduction").
 *
 * Accumulator overflow bugs may be specific to particular update lengths (in
 * blocks) and/or particular values of the previous acculumator.  Note that the
 * accumulator starts at 0 which gives the lowest chance of an overflow.  Thus,
 * a single all-one-bits test vector may be insufficient.
 *
 * Considering that, do the following test: continuously update a single
 * Poly1305 context with all-one-bits data of varying lengths (0, 16, 32, ...,
/* …（同文件无关代码省略）… */
static void test_poly1305_allones_keys_and_message(struct kunit *test)
{
	const size_t max_len = 4096;
	u8 *data = alloc_buf(test, max_len);
	struct poly1305_desc_ctx mac_ctx, macofmacs_ctx;
	u8 mac[POLY1305_DIGEST_SIZE];

	memset(data, 0xff, max_len);

	poly1305_init(&mac_ctx, data);
	poly1305_init(&macofmacs_ctx, data);
	for (int i = 0; i < 32; i++) {
		for (size_t len = 0; len <= max_len; len += 16) {
			struct poly1305_desc_ctx tmp_ctx;

			poly1305_update(&mac_ctx, data, len);
			tmp_ctx = mac_ctx;
			poly1305_final(&tmp_ctx, mac);
			poly1305_update(&macofmacs_ctx, mac,
					POLY1305_DIGEST_SIZE);
		}
	}
	poly1305_final(&macofmacs_ctx, mac);
	KUNIT_ASSERT_MEMEQ(test, mac, poly1305_allones_macofmacs,
			   POLY1305_DIGEST_SIZE);
}
/* …（同文件无关代码省略）… */
static void test_poly1305_reduction_edge_cases(struct kunit *test)
{
	static const u8 key[POLY1305_KEY_SIZE] = { 1 }; /* r_key=1, s_key=0 */
	u8 data[3 * POLY1305_BLOCK_SIZE] = {};
	u8 expected_mac[POLY1305_DIGEST_SIZE];
	u8 actual_mac[POLY1305_DIGEST_SIZE];

	for (int i = 1; i <= 10; i++) {
		/* Set the first data block to 2**128 - i. */
		data[0] = -i;
		memset(&data[1], 0xff, POLY1305_BLOCK_SIZE - 1);

		/*
		 * Assuming s_key=0, the expected MAC as an integer is
		 * (2**130 - i mod 2**130 - 5) + 0 mod 2**128.  If 1 <= i <= 5,
		 * that's 5 - i.  If 6 <= i <= 10, that's 2**128 - i.
		 */
		if (i <= 5) {
			expected_mac[0] = 5 - i;
			memset(&expected_mac[1], 0, POLY1305_DIGEST_SIZE - 1);
		} else {
			expected_mac[0] = -i;
			memset(&expected_mac[1], 0xff,
			       POLY1305_DIGEST_SIZE - 1);
		}

		/* Compute and verify the MAC. */
		poly1305(key, data, sizeof(data), actual_mac);
		KUNIT_ASSERT_MEMEQ(test, actual_mac, expected_mac,
				   POLY1305_DIGEST_SIZE);
	}
}

static struct kunit_case poly1305_test_cases[] = {
	HASH_KUNIT_CASES,
	KUNIT_CASE(test_poly1305_allones_keys_and_message),
	KUNIT_CASE(test_poly1305_reduction_edge_cases),
	KUNIT_CASE(benchmark_hash),
	{},
};
