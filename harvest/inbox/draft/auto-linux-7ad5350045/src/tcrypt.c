// AUTO-DRAFT from torvalds/linux PR #bf2325084583daaea91084b25069f121d5d8832b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
*/
#define get_driver_name(tfm_type, tfm) crypto_tfm_alg_driver_name(tfm_type ## _tfm(tfm))

/*
 * Used by test_cipher_speed()
 */
		sg_set_buf(&sg[k + 1], xbuf[k], rem);
}

static inline int do_one_aead_op(struct aead_request *req, int ret)
{
	struct crypto_wait *wait = req->base.data;

	return crypto_wait_req(ret, wait);
}

struct test_mb_aead_data {
	struct scatterlist sg[XBUFSIZE];
	struct scatterlist sgout[XBUFSIZE];
	struct aead_request *req;
	struct crypto_wait wait;
	char *xbuf[XBUFSIZE];
	char *xoutbuf[XBUFSIZE];
	char *axbuf[XBUFSIZE];
	/* Fire up a bunch of concurrent requests */
	for (i = 0; i < num_mb; i++) {
		if (enc == ENCRYPT)
			rc[i] = crypto_aead_encrypt(data[i].req);
		else
			rc[i] = crypto_aead_decrypt(data[i].req);
	}

	/* Wait for all requests to finish */
	for (i = 0; i < num_mb; i++) {
		rc[i] = crypto_wait_req(rc[i], &data[i].wait);

		if (rc[i]) {
			pr_info("concurrent request %d error %d\n", i, rc[i]);
		}

	for (i = 0; i < num_mb; ++i) {
		data[i].req = aead_request_alloc(tfm, GFP_KERNEL);
		if (!data[i].req) {
			pr_err("alg: aead: Failed to allocate request for %s\n",
			       algo);
			while (i--)
				aead_request_free(data[i].req);
			goto out_free_xoutbuf;
		}
	}

	for (i = 0; i < num_mb; ++i) {
		crypto_init_wait(&data[i].wait);
		aead_request_set_callback(data[i].req,
					  CRYPTO_TFM_REQ_MAY_BACKLOG,
					  crypto_req_done, &data[i].wait);
	}

	pr_info("testing speed of multibuffer %s (%s) %s\n", algo,
					     bs + (enc ? authsize : 0),
					     assoc, aad_size);

				aead_request_set_ad(cur->req, aad_size);

				if (!enc) {

					aead_request_set_crypt(cur->req,
							       cur->sgout,
							       cur->sg,
							       bs, iv);
					ret = crypto_aead_encrypt(cur->req);
					ret = do_one_aead_op(cur->req, ret);

					if (ret) {
						pr_err("calculating auth failed (%d)\n",
					}
				}

				aead_request_set_crypt(cur->req, cur->sg,
						       cur->sgout, bs +
	
