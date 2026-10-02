// AUTO-DRAFT from torvalds/linux PR #bd35955b089ad881f57a5a2619687c804d057fec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
struct aes_gcm_key {
	/* Expanded AES key and the AES key length in bytes */
	struct aes_enckey aes_key;

	/* RFC4106 nonce (used only by the rfc4106 algorithms) */
	u32 rfc4106_nonce;
};
/* …（同文件无关代码省略）… */
#define AES_GCM_KEY_AESNI(key)	\
	container_of((key), struct aes_gcm_key_aesni, base)
/* …（同文件无关代码省略）… */
#define AES_GCM_KEY_VAES_AVX2(key) \
	container_of((key), struct aes_gcm_key_vaes_avx2, base)
/* …（同文件无关代码省略）… */
#define AES_GCM_KEY_VAES_AVX512(key) \
	container_of((key), struct aes_gcm_key_vaes_avx512, base)
/* …（同文件无关代码省略）… */
#define FLAG_AVX	BIT(2)
#define FLAG_VAES_AVX2	BIT(3)
#define FLAG_VAES_AVX512 BIT(4)
/* …（同文件无关代码省略）… */
static void aes_gcm_aad_update(const struct aes_gcm_key *key, u8 ghash_acc[16],
			       const u8 *aad, int aadlen, int flags)
{
	if (flags & FLAG_VAES_AVX512)
		aes_gcm_aad_update_vaes_avx512(AES_GCM_KEY_VAES_AVX512(key),
					       ghash_acc, aad, aadlen);
	else if (flags & FLAG_VAES_AVX2)
		aes_gcm_aad_update_vaes_avx2(AES_GCM_KEY_VAES_AVX2(key),
					     ghash_acc, aad, aadlen);
	else if (flags & FLAG_AVX)
		aes_gcm_aad_update_aesni_avx(AES_GCM_KEY_AESNI(key), ghash_acc,
					     aad, aadlen);
	else
		aes_gcm_aad_update_aesni(AES_GCM_KEY_AESNI(key), ghash_acc,
					 aad, aadlen);
}
/* …（同文件无关代码省略）… */
		unsigned int len;
		const u8 *src = walk.addr;

		if (unlikely(pos)) {
			len = min(len_this_step, 16 - pos);
			memcpy(&buf[pos], src, len);
/* …（同文件无关代码省略）… */
			kernel_fpu_end();
			kernel_fpu_begin();
		}
		assoclen -= orig_len_this_step;
	}
	if (unlikely(pos))
		aes_gcm_aad_update(key, ghash_acc, buf, pos, flags);
