// AUTO-DRAFT from torvalds/linux PR #ca98aabb221067856c213a5d830d0dbf40656e0a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
  // <<< BUG ANCHOR
#define SOL_TLS 282
/* …（同文件无关代码省略）… */
struct tls_crypto_info_keys {
	union {
		struct tls_crypto_info crypto_info;
		struct tls12_crypto_info_aes_gcm_128 aes128;
		struct tls12_crypto_info_chacha20_poly1305 chacha20;
		struct tls12_crypto_info_sm4_gcm sm4gcm;
		struct tls12_crypto_info_sm4_ccm sm4ccm;
		struct tls12_crypto_info_aes_ccm_128 aesccm128;
		struct tls12_crypto_info_aes_gcm_256 aesgcm256;
		struct tls12_crypto_info_aria_gcm_128 ariagcm128;
		struct tls12_crypto_info_aria_gcm_256 ariagcm256;
	};
	size_t len;
};
/* …（同文件无关代码省略）… */
static void tls_crypto_info_init(uint16_t tls_version, uint16_t cipher_type,
				 struct tls_crypto_info_keys *tls12,
				 char key_generation)
{
	memset(tls12, key_generation, sizeof(*tls12));
	memset(tls12, 0, sizeof(struct tls_crypto_info));

	switch (cipher_type) {
	case TLS_CIPHER_CHACHA20_POLY1305:
		tls12->len = sizeof(struct tls12_crypto_info_chacha20_poly1305);
		tls12->chacha20.info.version = tls_version;
		tls12->chacha20.info.cipher_type = cipher_type;
		break;
	case TLS_CIPHER_AES_GCM_128:
		tls12->len = sizeof(struct tls12_crypto_info_aes_gcm_128);
		tls12->aes128.info.version = tls_version;
		tls12->aes128.info.cipher_type = cipher_type;
		break;
	case TLS_CIPHER_SM4_GCM:
		tls12->len = sizeof(struct tls12_crypto_info_sm4_gcm);
		tls12->sm4gcm.info.version = tls_version;
		tls12->sm4gcm.info.cipher_type = cipher_type;
		break;
	case TLS_CIPHER_SM4_CCM:
		tls12->len = sizeof(struct tls12_crypto_info_sm4_ccm);
		tls12->sm4ccm.info.version = tls_version;
		tls12->sm4ccm.info.cipher_type = cipher_type;
		break;
	case TLS_CIPHER_AES_CCM_128:
		tls12->len = sizeof(struct tls12_crypto_info_aes_ccm_128);
		tls12->aesccm128.info.version = tls_version;
		tls12->aesccm128.info.cipher_type = cipher_type;
		break;
	case TLS_CIPHER_AES_GCM_256:
		tls12->len = sizeof(struct tls12_crypto_info_aes_gcm_256);
		tls12->aesgcm256.info.version = tls_version;
		tls12->aesgcm256.info.cipher_type = cipher_type;
		break;
	case TLS_CIPHER_ARIA_GCM_128:
		tls12->len = sizeof(struct tls12_crypto_info_aria_gcm_128);
		tls12->ariagcm128.info.version = tls_version;
		tls12->ariagcm128.info.cipher_type = cipher_type;
		break;
	case TLS_CIPHER_ARIA_GCM_256:
		tls12->len = sizeof(struct tls12_crypto_info_aria_gcm_256);
		tls12->ariagcm256.info.version = tls_version;
		tls12->ariagcm256.info.cipher_type = cipher_type;
		break;
	default:
		break;
	}
}
/* …（同文件无关代码省略）… */
static void ulp_sock_pair(struct __test_metadata *_metadata,
			  int *fd, int *cfd, bool *notls)
{
	struct sockaddr_in addr;
	socklen_t len;
	int sfd, ret;

	*notls = false;
	len = sizeof(addr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = 0;

	*fd = socket(AF_INET, SOCK_STREAM, 0);
	sfd = socket(AF_INET, SOCK_STREAM, 0);

	ret = bind(sfd, &addr, sizeof(addr));
	ASSERT_EQ(ret, 0);
	ret = listen(sfd, 10);
	ASSERT_EQ(ret, 0);

	ret = getsockname(sfd, &addr, &len);
	ASSERT_EQ(ret, 0);

	ret = connect(*fd, &addr, sizeof(addr));
	ASSERT_EQ(ret, 0);

	*cfd = accept(sfd, &addr, &len);
	ASSERT_GE(*cfd, 0);

	close(sfd);

	ret = setsockopt(*fd, IPPROTO_TCP, TCP_ULP, "tls", sizeof("tls"));
	if (ret != 0) {
		ASSERT_EQ(errno, ENOENT);
		*notls = true;
		printf("Failure setting TCP_ULP, testing without tls\n");
		return;
	}

	ret = setsockopt(*cfd, IPPROTO_TCP, TCP_ULP, "tls", sizeof("tls"));
	ASSERT_EQ(ret, 0);
}
/* …（同文件无关代码省略）… */
FIXTURE_TEARDOWN(tls_basic)
{
	close(self->fd);
	close(self->cfd);
}
/* …（同文件无关代码省略）… */
	},
};

FIXTURE(zero_len)
{
	int fd, cfd;
/* …（同文件无关代码省略）… */

FIXTURE_SETUP(zero_len)
{
	struct tls_crypto_info_keys tls12;
	int ret;

	tls_crypto_info_init(TLS_1_2_VERSION, TLS_CIPHER_AES_CCM_128,
			     &tls12, 0);

	ulp_sock_pair(_metadata, &self->fd, &self->cfd, &self->notls);
	if (self->notls)
		return;

	/* Don't install keys on fd, we'll send raw records */
	ret = setsockopt(self->cfd, SOL_TLS, TLS_RX, &tls12, tls12.len);
	ASSERT_EQ(ret, 0);
}

FIXTURE_TEARDOWN(zero_len)
/* …（同文件无关代码省略）… */
	int rec_off;
	int i;

	for (i = 0; i < 4 && variant->recs[i]; i++)
		EXPECT_EQ(send(self->fd, variant->recs[i]->cipher_data,
			       variant->recs[i]->cipher_len, 0),
			  variant->recs[i]->cipher_len);

	rec = &variant->recs[0];
	rec_off = 0;
/* …（同文件无关代码省略）… */
	}
};

FIXTURE(tls_err)
{
	int fd, cfd;
