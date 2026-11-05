// AUTO-DRAFT from postgres/postgres PR #ee7cfb99a34cfba1cffdd456dc269243ddc69f3b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
void
ExprEvalPushStep(ExprState *es, const ExprEvalStep *s)
{
	if (es->steps_alloc == 0)
	{
		es->steps_alloc = 16;
		es->steps = palloc_array(ExprEvalStep, es->steps_alloc);
	}
	else if (es->steps_alloc == es->steps_len)
	{
		es->steps_alloc *= 2;
		es->steps = repalloc_array(es->steps, ExprEvalStep, es->steps_alloc);
	}

	memcpy(&es->steps[es->steps_len++], s, sizeof(ExprEvalStep));
}
/* …（同文件无关代码省略）… */
		jsestate->args = lappend(jsestate->args, var);
	}

	/* Step for jsonpath evaluation; see ExecEvalJsonExprPath(). */
	scratch->opcode = EEOP_JSONEXPR_PATH;
	scratch->resvalue = resv;
	scratch->resnull = resnull;
	scratch->d.jsonexpr.jsestate = jsestate;
	ExprEvalPushStep(state, scratch);

	/*
	 * Step to return NULL after jumping to skip the EEOP_JSONEXPR_PATH step
	 * when either formatted_expr or pathspec is NULL.  Adjust jump target
	 * addresses of JUMPs that we added above.
	 */
	foreach(lc, jumps_return_null)
	{
		ExprEvalStep *as = &state->steps[lfirst_int(lc)];

		as->d.jump.jumpdone = state->steps_len;
	}
	scratch->opcode = EEOP_CONST;
	scratch->resvalue = resv;
	scratch->resnull = resnull;
	scratch->d.constval.value = (Datum) 0;
	scratch->d.constval.isnull = true;
	ExprEvalPushStep(state, scratch);

	escontext = jsexpr->on_error->btype != JSON_BEHAVIOR_ERROR ?
/* …（同文件无关代码省略）… */

	/*
	 * Steps to coerce the result value computed by EEOP_JSONEXPR_PATH or the
	 * NULL returned on NULL input as described above.
	 */
	jsestate->jump_eval_coercion = -1;
	if (jsexpr->use_json_coercion)
