// AUTO-DRAFT from postgres/postgres PR #1c9efb57ec974b9c84df041644a39d8d4cc03004
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		pqTraceOutputMessage(conn, conn->inBuffer + conn->inStart, false);

	/* Only try to acquire credentials if GSS delegation isn't disabled. */
	if (!pg_GSS_have_cred_cache(&conn->gcred))
		conn->gcred = GSS_C_NO_CREDENTIAL;

	if (conn->gssdelegation && conn->gssdelegation[0] == '1')
		gss_flags |= GSS_C_DELEG_FLAG;

	maj_stat = gss_init_sec_context(&min_stat,
									conn->gcred,
