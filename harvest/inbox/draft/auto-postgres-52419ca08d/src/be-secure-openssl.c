// AUTO-DRAFT from postgres/postgres PR #e9526740926a801554cc92ea4d5620fca3546fe8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static int	ssl_set_port_bio(Port *port);

static DH  *load_dh_file(char *filename, bool isServerStart);
static DH  *load_dh_buffer(const char *buffer, size_t len);
static int	ssl_external_passwd_cb(char *buf, int size, int rwflag, void *userdata);
static int	dummy_ssl_passwd_cb(char *buf, int size, int rwflag, void *userdata);
static int	verify_cb(int ok, X509_STORE_CTX *ctx);
/* …（同文件无关代码省略）… */
static int
port_bio_read(BIO *h, char *buf, int size)
{
	int			res = 0;
	Port	   *port = (Port *) BIO_get_data(h);

	if (buf != NULL)
	{
		res = (int) secure_raw_read(port, buf, size);
		BIO_clear_retry_flags(h);
		port->last_read_was_eof = res == 0;
		if (res <= 0)
		{
			/* If we were interrupted, tell caller to retry */
			if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
			{
				BIO_set_retry_read(h);
			}
		}
	}

	return res;
}
/* …（同文件无关代码省略）… */
static int
port_bio_write(BIO *h, const char *buf, int size)
{
	int			res = 0;

	res = (int) secure_raw_write(((Port *) BIO_get_data(h)), buf, size);
	BIO_clear_retry_flags(h);
	if (res <= 0)
	{
		/* If we were interrupted, tell caller to retry */
		if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
		{
			BIO_set_retry_write(h);
		}
	}

	return res;
}
/* …（同文件无关代码省略）… */
static long
port_bio_ctrl(BIO *h, int cmd, long num, void *ptr)
{
	long		res;
	Port	   *port = (Port *) BIO_get_data(h);

	switch (cmd)
	{
		case BIO_CTRL_EOF:

			/*
			 * This should not be needed. port_bio_read already has a way to
			 * signal EOF to OpenSSL. However, OpenSSL made an undocumented,
			 * backwards-incompatible change and now expects EOF via BIO_ctrl.
			 * See https://github.com/openssl/openssl/issues/8208
			 */
			res = port->last_read_was_eof;
			break;
		case BIO_CTRL_FLUSH:
			/* libssl expects all BIOs to support BIO_flush. */
			res = 1;
			break;
		default:
			res = 0;
			break;
	}

	return res;
}
/* …（同文件无关代码省略）… */
static BIO_METHOD *
port_bio_method(void)
{
	if (!port_bio_method_ptr)
	{
		int			my_bio_index;

		my_bio_index = BIO_get_new_index();
		if (my_bio_index == -1)
			return NULL;
		my_bio_index |= BIO_TYPE_SOURCE_SINK;
		port_bio_method_ptr = BIO_meth_new(my_bio_index, "PostgreSQL backend socket");
		if (!port_bio_method_ptr)
			return NULL;
		if (!BIO_meth_set_write(port_bio_method_ptr, port_bio_write) ||
			!BIO_meth_set_read(port_bio_method_ptr, port_bio_read) ||
			!BIO_meth_set_ctrl(port_bio_method_ptr, port_bio_ctrl))
		{
			BIO_meth_free(port_bio_method_ptr);
			port_bio_method_ptr = NULL;
			return NULL;
		}
	}
	return port_bio_method_ptr;
}
/* …（同文件无关代码省略）… */
static int
ssl_set_port_bio(Port *port)
{
	BIO		   *bio;
	BIO_METHOD *bio_method;

	bio_method = port_bio_method();
	if (bio_method == NULL)
		return 0;

	bio = BIO_new(bio_method);
	if (bio == NULL)
		return 0;

	BIO_set_data(bio, port);
	BIO_set_init(bio, 1);

	SSL_set_bio(port->ssl, bio, bio);
	return 1;
}
/* …（同文件无关代码省略）… */
static DH  *
load_dh_file(char *filename, bool isServerStart)
{
	FILE	   *fp;
	DH		   *dh = NULL;
	int			codes;

	/* attempt to open file.  It's not an error if it doesn't exist. */
	if ((fp = AllocateFile(filename, "r")) == NULL)
	{
		ereport(isServerStart ? FATAL : LOG,
				(errcode_for_file_access(),
				 errmsg("could not open DH parameters file \"%s\": %m",
						filename)));
		return NULL;
	}

	dh = PEM_read_DHparams(fp, NULL, NULL, NULL);
	FreeFile(fp);

	if (dh == NULL)
	{
		ereport(isServerStart ? FATAL : LOG,
				(errcode(ERRCODE_CONFIG_FILE_ERROR),
				 errmsg("could not load DH parameters file: %s",
						SSLerrmessage(ERR_get_error()))));
		return NULL;
	}

	/* make sure the DH parameters are usable */
	if (DH_check(dh, &codes) == 0)
	{
		ereport(isServerStart ? FATAL : LOG,
				(errcode(ERRCODE_CONFIG_FILE_ERROR),
				 errmsg("invalid DH parameters: %s",
						SSLerrmessage(ERR_get_error()))));
		DH_free(dh);
		return NULL;
	}
	if (codes & DH_CHECK_P_NOT_PRIME)
	{
		ereport(isServerStart ? FATAL : LOG,
				(errcode(ERRCODE_CONFIG_FILE_ERROR),
				 errmsg("invalid DH parameters: p is not prime")));
		DH_free(dh);
		return NULL;
	}
	if ((codes & DH_NOT_SUITABLE_GENERATOR) &&
		(codes & DH_CHECK_P_NOT_SAFE_PRIME))
	{
		ereport(isServerStart ? FATAL : LOG,
				(errcode(ERRCODE_CONFIG_FILE_ERROR),
				 errmsg("invalid DH parameters: neither suitable generator or safe prime")));
		DH_free(dh);
		return NULL;
	}

	return dh;
}

/*
 *	Load hardcoded DH parameters.
 *
 *	If DH parameters cannot be loaded from a specified file, we can load
 *	the hardcoded DH parameters supplied with the backend to prevent
 *	problems.
 */
static DH  *
load_dh_buffer(const char *buffer, size_t len)
{
	BIO		   *bio;
	DH		   *dh = NULL;

	bio = BIO_new_mem_buf(buffer, len);
	if (bio == NULL)
		return NULL;
	dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
	if (dh == NULL)
		ereport(DEBUG2,
				(errmsg_internal("DH load buffer: %s",
								 SSLerrmessage(ERR_get_error()))));
	BIO_free(bio);

	return dh;
}

/*
 *	Passphrase collection callback using ssl_passphrase_command
 */
static int
ssl_external_passwd_cb(char *buf, int size, int rwflag, void *userdata)
{
	/* same prompt as OpenSSL uses internally */
	const char *prompt = "Enter PEM pass phrase:";
	const char *cmd = userdata;

	Assert(rwflag == 0);

	return run_ssl_passphrase_command(cmd, prompt, ssl_is_server_start, buf, size);
}
/* …（同文件无关代码省略）… */
static int
dummy_ssl_passwd_cb(char *buf, int size, int rwflag, void *userdata)
{
	/* Set flag to change the error message we'll report */
	dummy_ssl_passwd_cb_called = true;
	/* And return empty string */
	Assert(size > 0);
	buf[0] = '\0';
	return 0;
}
/* …（同文件无关代码省略）… */
static char *
prepare_cert_name(char *name)
{
	size_t		namelen = strlen(name);
	char	   *truncated = name;

	/*
	 * Common Names are 64 chars max, so for a common case where the CN is the
	 * last field, 
