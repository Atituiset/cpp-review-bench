// AUTO-DRAFT from postgres/postgres PR #a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


	matstate->eof_underlying = false;
	matstate->tuplestorestate = NULL;

	/*
	 * Miscellaneous initialization
/* …（同文件无关代码省略）… */
void
ExecEndMaterial(MaterialState *node)
{
	/*
	 * Release tuplestore resources
	 */
/* …（同文件无关代码省略）… */
		node->eof_underlying = false;
	}
}
