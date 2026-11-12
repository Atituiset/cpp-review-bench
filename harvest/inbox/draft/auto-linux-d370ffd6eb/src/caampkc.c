// AUTO-DRAFT from torvalds/linux PR #0eaee8fc9305b4ae8e75e68c30e7e93b6b367234
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static void caam_rsa_free_key(struct caam_rsa_key *key)
{
	kfree_sensitive(key->d);
	kfree_sensitive(key->p);
	kfree_sensitive(key->q);
	kfree_sensitive(key->dp);
	kfree_sensitive(key->dq);
	kfree_sensitive(key->qinv);
	kfree_sensitive(key->tmp1);
	kfree_sensitive(key->tmp2);
	kfree(key->e);
	kfree(key->n);
	memset(key, 0, sizeof(*key));
}

static void caam_rsa_drop_leading_zeros(const u8 **ptr, size_t *nbytes)
{
	while (!**ptr && *nbytes) {
		(*ptr)++;
		(*nbytes)--;
	}
/* …（同文件无关代码省略）… */
 * @ptr   : pointer to {dP, dQ, qInv} CRT member
 * @nbytes: length in bytes of {dP, dQ, qInv} CRT member
 * @dstlen: length in bytes of corresponding p or q prime factor
 */
static u8 *caam_read_rsa_crt(const u8 *ptr, size_t nbytes, size_t dstlen)
{
	u8 *dst;

	caam_rsa_drop_leading_zeros(&ptr, &nbytes);
	if (!nbytes)
		return NULL;

	dst = kzalloc(dstlen, GFP_KERNEL);
	if (!dst)
		return NULL;

	memcpy(dst + (dstlen - nbytes), ptr, nbytes);

	return dst;
}

/**
/* …（同文件无关代码省略）… */
static inline u8 *caam_read_raw_data(const u8 *buf, size_t *nbytes)
{

	caam_rsa_drop_leading_zeros(&buf, nbytes);
	if (!*nbytes)
		return NULL;

	return kmemdup(buf, *nbytes, GFP_KERNEL);
}
/* …（同文件无关代码省略）… */
static int caam_rsa_check_key_length(unsigned int len)
{
	if (len > 4096)
		return -EINVAL;
	return 0;
}
/* …（同文件无关代码省略）… */
static int caam_rsa_set_priv_key_form(struct caam_rsa_ctx *ctx,
				       struct rsa_key *raw_key)
{
	struct caam_rsa_key *rsa_key = &ctx->key;
	size_t p_sz = raw_key->p_sz;
	size_t q_sz = raw_key->q_sz;
	unsigned aligned_size;

	rsa_key->p = caam_read_raw_data(raw_key->p, &p_sz);
	if (!rsa_key->p)
		return -ENOMEM;
	rsa_key->p_sz = p_sz;

	rsa_key->q = caam_read_raw_data(raw_key->q, &q_sz);
	if (!rsa_key->q)
		goto free_p;
	rsa_key->q_sz = q_sz;

	aligned_size = ALIGN(raw_key->p_sz, dma_get_cache_alignment());
	rsa_key->tmp1 = kzalloc(aligned_size, GFP_KERNEL);
	if (!rsa_key->tmp1)
		goto free_q;

	aligned_size = ALIGN(raw_key->q_sz, dma_get_cache_alignment());
	rsa_key->tmp2 = kzalloc(aligned_size, GFP_KERNEL);
	if (!rsa_key->tmp2)
		goto free_tmp1;

	rsa_key->priv_form = FORM2;

	rsa_key->dp = caam_read_rsa_crt(raw_key->dp, raw_key->dp_sz, p_sz);
	if (!rsa_key->dp)
		goto free_tmp2;

	rsa_key->dq = caam_read_rsa_crt(raw_key->dq, raw_key->dq_sz, q_sz);
	if (!rsa_key->dq)
		goto free_dp;

	rsa_key->qinv = caam_read_rsa_crt(raw_key->qinv, raw_key->qinv_sz,
					  q_sz);
	if (!rsa_key->qinv)
		goto free_dq;

	rsa_key->priv_form = FORM3;

	return 0;

free_dq:
	kfree_sensitive(rsa_key->dq);
	rsa_key->dq = NULL;
free_dp:
	kfree_sensitive(rsa_key->dp);
	rsa_key->dp = NULL;
free_tmp2:
	kfree_sensitive(rsa_key->tmp2);
	rsa_key->tmp2 = NULL;
free_tmp1:
	kfree_sensitive(rsa_key->tmp1);
	rsa_key->tmp1 = NULL;
free_q:
	kfree_sensitive(rsa_key->q);
	rsa_key->q = NULL;
free_p:
	kfree_sensitive(rsa_key->p);
	rsa_key->p = NULL;
	return -ENOMEM;
}

static int caam_rsa_set_priv_key(struct crypto_akcipher *tfm, const void *key,
				 unsigned int keylen)
{
	struct caam_rsa_ctx *ctx = akcipher_tfm_ctx_dma(tfm);
	struct rsa_key raw_key = {NULL};
	struct caam_rsa_key *rsa_key = &ctx->key;
	int ret;

	/* Free the old RSA key if any */
	caam_rsa_free_key(rsa_key);

	ret = rsa_parse_priv_key(&raw_key, key, keylen);
	if (ret)
		return ret;

	/* Copy key in DMA zone */
	rsa_key->d = kmemdup(raw_key.d, raw_key.d_sz, GFP_KERNEL);
	if (!rsa_key->d)
		goto err;

	rsa_key->e = kmemdup(raw_key.e, raw_key.e_sz, GFP_KERNEL);
	if (!rsa_key->e)
		goto err;

	/*
	 * Skip leading zeros and copy the positive integer to a buffer
	 * allocated in the GFP_KERNEL zone. The decryption descriptor
	 * expects a positive integer for the RSA modulus and uses its length as
	 * decryption output length.
	 */
	rsa_key->n = caam_read_raw_data(raw_key.n, &raw_key.n_sz);
	if (!rsa_key->n)
		goto err;

	if (caam_rsa_check_key_length(raw_key.n_sz << 3)) {
		caam_rsa_free_key(rsa_key);
		return -EINVAL;
	}

	rsa_key->d_sz = raw_key.d_sz;
	rsa_key->e_sz = raw_key.e_sz;
	rsa_key->n_sz = raw_key.n_sz;

	ret = caam_rsa_set_priv_key_form(ctx, &raw_key);
	if (ret)
		goto err;

	return 0;

err:
	caam_rsa_free_key(rsa_key);
	return -ENOMEM;
}

static unsigned int caam_rsa_max_size(struct crypto_akcipher *tfm)
{
	struct caam_rsa_ctx *ctx = akcipher_tfm_ctx_dma(tfm);

	return ctx->key.n_sz;
}
