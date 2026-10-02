// AUTO-DRAFT from torvalds/linux PR #bd35955b089ad881f57a5a2619687c804d057fec
#define AES_CRYPT_SG(crypt_func, dst, src, cryptlen, start_pos, ...)           \
	({                                                                     \
		unsigned int remaining = (cryptlen);                           \
		unsigned int spos = (start_pos);                               \
                                                                               \
		if (remaining != 0) {                                          \
			struct scatter_walk dst_walk, src_walk;                \
			u8 tmp[4 * AES_BLOCK_SIZE] __aligned(                  \
				__alignof__(long));                            \
                                                                               \
			scatterwalk_start_at_pos(&dst_walk, (dst), spos);      \
			scatterwalk_start_at_pos(&src_walk, (src), spos);      \
			do {                                                   \
				unsigned int dst_avail = scatterwalk_clamp(    \
					&dst_walk, remaining);                 \
				unsigned int src_avail = scatterwalk_clamp(    \
					&src_walk, remaining);                 \
				unsigned int n = min(dst_avail, src_avail);    \
				u8 *dst_virt;                                  \
				const u8 *src_virt;                            \
                                                                               \
				if (n < remaining) {                           \
					if (n < sizeof(tmp)) {                 \
						n = min(remaining,             \
							sizeof(tmp));          \
						memcpy_from_scatterwalk(       \
							tmp, &src_walk, n);    \
						crypt_func(tmp, tmp, n,        \
							   ##__VA_ARGS__);     \
						memcpy_to_scatterwalk(         \
							&dst_walk, tmp, n);    \
						remaining -= n;                \
						continue;                      \
					}                                      \
					n = round_down(n, AES_BLOCK_SIZE);     \
				}                                              \
                                                                               \
				scatterwalk_map(&dst_walk);                    \
				dst_virt = dst_walk.addr;                      \
				if (IS_ENABLED(CONFIG_HIGHMEM) &&              \
				    offset_in_page(src_walk.offset) ==         \
					    offset_in_page(dst_walk.offset) && \
				    sg_page(src_walk.sg) + (src_walk.offset /  \
							    PAGE_SIZE) ==      \
					    sg_page(dst_walk.sg) +             \
						    (dst_walk.offset /         \
						     PAGE_SIZE)) {             \
					src_virt = dst_virt;                   \
				} else {                                       \
					scatterwalk_map(&src_walk);            \
					src_virt = src_walk.addr;              \
				}                                              \
				crypt_func(dst_virt, src_virt, n,              \
					   ##__VA_ARGS__);                     \
				if (src_virt != dst_virt)                      \
					scatterwalk_unmap(&src_walk);          \
				scatterwalk_advance(&src_walk, n);             \
				scatterwalk_done_dst(&dst_walk, n);            \
				remaining -= n;                                \
			} while (remaining);                                   \
			memzero_explicit(tmp, sizeof(tmp));                    \
		}                                                              \
	})
/* …（同文件无关代码省略）… */
static void crypto_aes_cbc_decrypt_sg(struct skcipher_request *req,
				      unsigned int cryptlen,
				      const struct aes_key *key)
{
	AES_CRYPT_SG(aes_cbc_decrypt, req->dst, req->src, cryptlen, 0, req->iv,
		     key);
}
/* …（同文件无关代码省略）… */
static __maybe_unused int crypto_aes_cbc_decrypt(struct skcipher_request *req)
{
	const struct aes_key *key =
		crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));

	if (unlikely(req->cryptlen % AES_BLOCK_SIZE))
		return -EINVAL;
	crypto_aes_cbc_decrypt_sg(req, req->cryptlen, key);
	return 0;
}
/* …（同文件无关代码省略）… */
static __maybe_unused int crypto_aes_xctr_crypt(struct skcipher_request *req)
{
	const struct aes_enckey *key =
		crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	u64 ctr = 1;

	AES_CRYPT_SG(aes_xctr, req->dst, req->src, req->cryptlen, 0, &ctr,
		     req->iv, key);
	return 0;
}
/* …（同文件无关代码省略）… */
		.decrypt = crypto_aes_cbc_decrypt,
	},
#endif
#if IS_ENABLED(CONFIG_CRYPTO_CTS)
	{
		.base.cra_name = "cts(cbc(aes))",
		.base.cra_driver_name = "cts-cbc-aes-lib",
/* …（同文件无关代码省略）… */
		.decrypt = crypto_aes_xctr_crypt,
	},
#endif
#if IS_ENABLED(CONFIG_CRYPTO_XTS)
	{
		.base.cra_name = "xts(aes)",
		.base.cra_driver_name = "xts-aes-lib",
/* …（同文件无关代码省略）… */
}

static struct aead_alg aead_algs[] = {
#if IS_ENABLED(CONFIG_CRYPTO_GCM)
	{
		.base.cra_name = "gcm(aes)",
		.base.cra_driver_name = "gcm-aes-lib",
/* …（同文件无关代码省略）… */
		.chunksize = AES_BLOCK_SIZE,
	},
#endif /* CONFIG_CRYPTO_GCM */
#if IS_ENABLED(CONFIG_CRYPTO_CCM)
	{
		.base.cra_name = "ccm(aes)",
		.base.cra_driver_name = "ccm-aes-lib",
