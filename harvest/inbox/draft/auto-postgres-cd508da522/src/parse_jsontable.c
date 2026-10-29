// AUTO-DRAFT from postgres/postgres PR #35b349a530c61628f4b7a7968212e594297523c6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	/*
	 * Transform JsonFuncExpr representing the top JSON_TABLE context_item and
	 * pathspec into a dummy JSON_TABLE_OP JsonExpr.
	 */
	jfe = makeNode(JsonFuncExpr);
	jfe->op = JSON_TABLE_OP;
	jfe->context_item = jt->context_item;
	jfe->pathspec = (Node *) rootPathSpec->string;
	jfe->passing = jt->passing;
	jfe->on_empty = NULL;
	jfe->on_error = jt->on_error;
	jfe->location = jt->location;
/* …（同文件无关代码省略）… */
		pathspec = makeStringConst(path.data, -1);
	}
	jfexpr->pathspec = pathspec;
	jfexpr->passing = passingArgs;
	jfexpr->output = makeNode(JsonOutput);
	jfexpr->output->typeName = jtc->typeName;
	jfexpr->output->returning = makeNode(JsonReturning);
