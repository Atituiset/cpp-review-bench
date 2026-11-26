// AUTO-DRAFT from postgres/postgres PR #73401447c3404aa8bebf8a4c89d9e604dc6797c4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>

#define SSL_ERR_LEN 128
/* …（同文件无关代码省略）… */

	/*
	 * Server aborted the connection with TLS "no_application_protocol" alert.
	 * The ERR_reason_error_string() function doesn't give any error string
	 * for that for some reason, so do it ourselves.  See
	 * https://github.com/openssl/openssl/issues/24300.  This is available in
	 * OpenSSL 1.1.0 and later, as well as in LibreSSL 3.4.3 (OpenBSD 7.0) and
	 * later.
	 */
#ifdef SSL_AD_NO_APPLICATION_PROTOCOL
	if (ERR_GET_LIB(ecode) == ERR_LIB_SSL &&
		ERR_GET_REASON(ecode) == SSL_AD_REASON_OFFSET + SSL_AD_NO_APPLICATION_PROTOCOL)
	{
		snprintf(errbuf, SSL_ERR_LEN, "no application protocol");
		return errbuf;
	}
#endif

	/*
	 * In OpenSSL 3.0.0 and later, ERR_reason_error_string does not map system
