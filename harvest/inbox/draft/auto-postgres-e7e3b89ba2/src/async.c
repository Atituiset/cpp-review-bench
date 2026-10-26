// AUTO-DRAFT from postgres/postgres PR #dfd7ede529f288639760444a9c69f3f7bb65450c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "utils/ps_status.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"


/*
/* …（同文件无关代码省略）… */
typedef struct ChannelName
{
	char		channel[NAMEDATALEN];	/* hash key */
} ChannelName;
/* …（同文件无关代码省略）… */
/*
 * SQL function: return a set of the channel names this backend is actively
 * listening to.
 *
 * Note: this coding relies on the fact that the localChannelTable cannot
 * change within a transaction.
 */
Datum
pg_listening_channels(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	HASH_SEQ_STATUS *status;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL())
	{
		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* Initialize hash table iteration if we have any channels */
		if (localChannelTable != NULL)
		{
			MemoryContext oldcontext;

			oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
			status = palloc_object(HASH_SEQ_STATUS);
			hash_seq_init(status, localChannelTable);
			funcctx->user_fctx = status;
			MemoryContextSwitchTo(oldcontext);
		}
		else
		{
			funcctx->user_fctx = NULL;
		}
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	status = (HASH_SEQ_STATUS *) funcctx->user_fctx;

	if (status != NULL)
	{
		ChannelName *entry;

		entry = (ChannelName *) hash_seq_search(status);
		if (entry != NULL)
			SRF_RETURN_NEXT(funcctx, CStringGetTextDatum(entry->channel));
	}

	SRF_RETURN_DONE(funcctx);
}

/*
