// AUTO-DRAFT from postgres/postgres PR #1c9efb57ec974b9c84df041644a39d8d4cc03004
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
						}
						if (conn->gcred == GSS_C_NO_CREDENTIAL)
						{
							if (!pg_GSS_have_cred_cache(&conn->gcred))
							{
								libpq_append_conn_error(conn,
														"GSSAPI encryption required but no credential cache");
/* …（同文件无关代码省略）… */
	if ((remaining_methods & ENC_GSSAPI) != 0)
	{
		/*
		 * If GSSAPI encryption is enabled, then call pg_GSS_have_cred_cache()
		 * which will return true if we can acquire credentials (and give us a
		 * handle to use in conn->gcred), and then send a packet to the server
		 * asking for GSSAPI Encryption (and skip past SSL negotiation and
		 * regular startup below).
		 */
		if (!conn->gctx)
		{
			if (!pg_GSS_have_cred_cache(&conn->gcred))
			{
				conn->allowed_enc_methods &= ~ENC_GSSAPI;
				remaining_methods &= ~ENC_GSSAPI;
/* …（同文件无关代码省略）… */
	conn->altsock = PGINVALID_SOCKET;
	conn->Pfdebug = NULL;

	/*
	 * We try to send at least 8K at a time, which is the usual size of pipe
	 * buffers on Unix systems.  That way, when we are sending a large amount
