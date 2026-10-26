# auto-postgres-e7e3b89ba2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#dfd7ede529f288639760444a9c69f3f7bb65450c](https://github.com/postgres/postgres/commit/dfd7ede529f288639760444a9c69f3f7bb65450c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #dfd7ede529f288639760444a9c69f3f7bb65450c (https://github.com/postgres/postgres/commit/dfd7ede529f288639760444a9c69f3f7bb65450c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1107；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT dfd7ede529f288639760444a9c69f3f7bb65450c Fix leaked hash scans in pg_listening_channels(). :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -188,6 +188,7 @@
 #include "utils/ps_status.h"
 #include "utils/snapmgr.h"
 #include "utils/timestamp.h"
+#include "utils/tuplestore.h"
 
 
 /*
@@ -1087,53 +1088,38 @@ Async_UnlistenAll(void)
 /*
  * SQL function: return a set of the channel names this backend is actively
  * listening to.
- *
- * Note: this coding relies on the fact that the localChannelTable cannot
- * change within a transaction.
  */
 Datum
 pg_listening_channels(PG_FUNCTION_ARGS)
 {
-	FuncCallContext *funcctx;
-	HASH_SEQ_STATUS *status;
+	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
+
+	InitMaterializedSRF(fcinfo, MAT_SRF_USE_EXPECTED_DESC);
 
-	/* stuff done only on the first call of the function */
-	if (SRF_IS_FIRSTCALL())
+	/*
+	 * Complete the hash scan in one call, since the caller might not fetch
+	 * all the rows and would otherwise leave the scan registered until
+	 * transaction end.
+	 */
+	if (localChannelTable != NULL)
 	{
-		/* create a function context for cross-call persistence */
-		funcctx = SRF_FIRSTCALL_INIT();
+		HASH_SEQ_STATUS status;
+		ChannelName *channelEntry;
 
-		/* Initialize hash table iteration if we have any channels */
-		if (localChannelTable != NULL)
+		hash_seq_init(&status, localChannelTable);
+		while ((channelEntry = (ChannelName *) hash_seq_search(&status)) != NULL)
 		{
-			MemoryContext oldcontext;
+			Datum		values[1];
+			bool		nulls[1];
 
-			oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
-			status = palloc_object(HASH_SEQ_STATUS);
-			hash_seq_init(status, localChannelTable);
-			funcctx->user_fctx = status;
-			MemoryContextSwitchTo(oldcontext);
+			values[0] = CStringGetTextDatum(channelEntry->channel);
+			nulls[0] = false;
+			tuplestore_putvalues(rsinfo->setResult, rsinfo->setDesc,
+								 values, nulls);
 		}
-		else
-		{
-			funcctx->user_fctx = NULL;
-		}
-	}
-
-	/* stuff done on every call of the function */
-	funcctx = SRF_PERCALL_SETUP();
-	status = (HASH_SEQ_STATUS *) funcctx->user_fctx;
-
-	if (status != NULL)
-	{
-		ChannelName *entry;
-
-		entry = (ChannelName *) hash_seq_search(status);
-		if (entry != NULL)
-			SRF_RETURN_NEXT(funcctx, CStringGetTextDatum(entry->channel));
 	}
 
-	SRF_RETURN_DONE(funcctx);
+	return (Datum) 0;
 }
 
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`hash_seq_init`
- 外部函数：`hash_seq_search`
- 外部函数：`palloc_object`
- 大写宏：`HASH_SEQ_STATUS`
- 大写宏：`NAMEDATALEN`
- 大写宏：`NULL`
- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`SQL`
- 大写宏：`SRF_FIRSTCALL_INIT`
- 大写宏：`SRF_IS_FIRSTCALL`
- 大写宏：`SRF_PERCALL_SETUP`
- 大写宏：`SRF_RETURN_DONE`
- 大写宏：`SRF_RETURN_NEXT`
- 外部类型：`ChannelName`
- 外部类型：`Datum`
- 外部类型：`FuncCallContext`
- 外部类型：`Initialize`
- 外部类型：`MemoryContext`
- 外部类型：`Note`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-e7e3b89ba2` → 本草稿移入 `cases/defect/auto-postgres-e7e3b89ba2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
