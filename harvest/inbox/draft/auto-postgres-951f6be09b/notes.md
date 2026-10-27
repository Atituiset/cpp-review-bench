# auto-postgres-951f6be09b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#e5d5d1ad1e3aa495609e19fa716dcea6a39db1bb](https://github.com/postgres/postgres/commit/e5d5d1ad1e3aa495609e19fa716dcea6a39db1bb) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e5d5d1ad1e3aa495609e19fa716dcea6a39db1bb (https://github.com/postgres/postgres/commit/e5d5d1ad1e3aa495609e19fa716dcea6a39db1bb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e5d5d1ad1e3aa495609e19fa716dcea6a39db1bb Add custom extension states to QueryDesc :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -23,18 +23,22 @@
 #include "executor/tstoreReceiver.h"
 #include "miscadmin.h"
 #include "pg_trace.h"
+#include "port/pg_bitutils.h"
 #include "tcop/pquery.h"
 #include "tcop/utility.h"
 #include "utils/memutils.h"
 #include "utils/snapmgr.h"
 
-
 /*
  * ActivePortal is the currently executing Portal (the most closely nested,
  * if there are several).
  */
 Portal		ActivePortal = NULL;
 
+/* Private data for QueryDesc.extension_state. */
+static const char **QueryDescExtensionNameArray = NULL;
+static int	QueryDescExtensionNamesAssigned = 0;
+static int	QueryDescExtensionNamesAllocated = 0;
 
 static void ProcessQuery(PlannedStmt *plan,
 						 const char *sourceText,
@@ -97,6 +101,10 @@ CreateQueryDesc(PlannedStmt *plannedstmt,
 	/* not yet executed */
 	qd->already_executed = false;
 
+	/* no extension state yet */
+	qd->extension_state = NULL;
+	qd->extension_state_allocated = 0;
+
 	return qd;
 }
 
@@ -113,10 +121,105 @@ FreeQueryDesc(QueryDesc *qdesc)
 	UnregisterSnapshot(qdesc->snapshot);
 	UnregisterSnapshot(qdesc->crosscheck_snapshot);
 
-	/* Only the QueryDesc itself need be freed */
+	/* Free extension state pointers */
+	if (qdesc->extension_state != NULL)
+		pfree(qdesc->extension_state);
+
+	/* Free the QueryDesc itself */
 	pfree(qdesc);
 }
 
+/*
+ * Map the name of an extension to an integer ID, to be used with
+ * SetQueryDescExtensionState() and GetQueryDescExtensionState().
+ *
+ * Within the lifetime of a particular backend, the same name will be mapped
+ * to the same ID every time.  IDs are not stable across backends.
+ */
+int
+GetQueryDescExtensionId(const char *extension_name)
+{
+	/* Search for an existing extension by this name; if found, return ID. */
+	for (int i = 0; i < QueryDescExtensionNamesAssigned; ++i)
+		if (strcmp(QueryDescExtensionNameArray[i], extension_name) == 0)
+			return i;
+
+	/* If there is no array yet, create one. */
+	if (QueryDescExtensionNameArray == NULL)
+	{
+		QueryDescExtensionNamesAllocated = 16;
+		QueryDescExtensionNameArray = (const char **)
+			MemoryContextAlloc(TopMemoryContext,
+							   QueryDescExtensionNamesAllocated
+							   * sizeof(char *));
+	}
+
+	/* If there's an array but it's currently full, expand it. */
+	if (QueryDescExtensionNamesAssigned >= QueryDescExtensionNamesAllocated)
+	{
+		int			i = pg_nextpower2_32(QueryDescExtensionNamesAssigned + 1);
+
+		QueryDescExtensionNameArray =
+			repalloc_array(QueryDescExtensionNameArray, const char *, i);
+		QueryDescExtensionNamesAllocated = i;
+	}
+
+	/* Assign and return new ID.  The name is copied, to be on the safe side. */
+	QueryDescExtensionNameArray[QueryDescExtensionNamesAssigned] =
+		MemoryContextStrdup(TopMemoryContext, extension_name);
+	return QueryDescExtensionNamesAssigned++;
+}
+
+/*
+ * Get extension-specific state from a QueryDesc, or NULL if none was set.
+ */
+void *
+GetQueryDescExtensionState(QueryDesc *qdesc, int extension_id)
+{
+	Assert(extension_id >= 0);
+
+	if (extension_id >= qdesc->extension_state_allocated)
+		return NULL;
+
+	return qdesc->extension_state[extension_id];
+}
+
+/*
+ * Store extension-specific state into a QueryDesc.
+ *
+ * The array of pointers is allocated in the memory context of the QueryDesc,
+ * and is freed by FreeQueryDesc().
+ */
+void
+SetQueryDescExtensionState(QueryDesc *qdesc, int extension_id, void *opaque)
+{
+	Assert(extension_id >= 0);
+
+	/* If there is no array yet, create one. */
+	if (qdesc->extension_state == NULL)
+	{
+		qdesc->extension_state_allocated =
+			Max(4, pg_nextpower2_32(extension_id + 1));
+		qdesc->extension_state =
+			MemoryContextAllocZero(GetMemoryChunkContext(qdesc),
+								   qdesc->extension_state_allocated
+								   * sizeof(void *));
+	}
+
+	/* If there's an array but it's currently full, expand it. */
+	if (extension_id >= qdesc->extension_state_allocated)
+	{
+		int			i = pg_nextpower2_32(extension_id + 1);
+
+		qdesc->extension_state =
+			repalloc0_array(qdesc->extension_state, void *,
+							qdesc->ex
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`palloc_object`
- 外部函数：`pfree`
- 大写宏：`CMDTAG_DELETE`
- 大写宏：`CMDTAG_INSERT`
- 大写宏：`CMDTAG_MERGE`
- 大写宏：`CMDTAG_SELECT`
- 大写宏：`CMDTAG_UNKNOWN`
- 大写宏：`CMDTAG_UPDATE`
- 大写宏：`CMD_DELETE`
- 大写宏：`CMD_INSERT`
- 大写宏：`CMD_MERGE`
- 大写宏：`CMD_SELECT`
- 大写宏：`CMD_UPDATE`
- 大写宏：`NULL`
- 外部类型：`ActivePortal`
- 外部类型：`Build`
- 外部类型：`Call`
- 外部类型：`Can`
- 外部类型：`CommandTag`
- 外部类型：`Create`
- 外部类型：`DestReceiver`
- 外部类型：`ExecutorStart`
- 外部类型：`ForwardScanDirection`
- 外部类型：`InvalidSnapshot`
- 外部类型：`Now`
- 外部类型：`Only`
- 外部类型：`ParamListInfo`
- 外部类型：`PlannedStmt`
- 外部类型：`Portal`
- 外部类型：`ProcessQuery`
- 外部类型：`QueryCompletion`
- 外部类型：`QueryDesc`
- 外部类型：`QueryEnvironment`
- 外部类型：`Run`
- 外部类型：`Snapshot`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-951f6be09b` → 本草稿移入 `cases/defect/auto-postgres-951f6be09b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
