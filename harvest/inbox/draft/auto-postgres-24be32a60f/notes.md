# auto-postgres-24be32a60f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#c712b0d49b971fe7cf521c057bc7adb63b17760c](https://github.com/postgres/postgres/commit/c712b0d49b971fe7cf521c057bc7adb63b17760c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c712b0d49b971fe7cf521c057bc7adb63b17760c (https://github.com/postgres/postgres/commit/c712b0d49b971fe7cf521c057bc7adb63b17760c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c712b0d49b971fe7cf521c057bc7adb63b17760c Allow unknown-size shmem attachments in single-user mode :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -20,6 +20,7 @@
 #include "fmgr.h"
 #include "miscadmin.h"
 #include "storage/shmem.h"
+#include "utils/builtins.h"
 #include "utils/guc.h"
 #include "utils/injection_point.h"
 
@@ -129,3 +130,57 @@ get_test_shmem_attach_count(PG_FUNCTION_ARGS)
 		elog(ERROR, "shmem area not yet initialized");
 	PG_RETURN_INT32(TestShmem->attach_count);
 }
+
+
+/*
+ * Callback for test_shmem_register().  test_shmem_register() provides the
+ * options, we just pass them through to ShmemRequestStruct.
+ */
+static void
+test_shmem_after_startup_request(void *arg)
+{
+	ShmemStructOpts *opts = (ShmemStructOpts *) arg;
+
+	elog(LOG, "test_shmem_after_startup_request callback called");
+
+	ShmemRequestStructWithOpts(opts);
+}
+
+/*
+ * Allocate or attach to a shmem structure, with the caller-supplied name and
+ * size.
+ *
+ * The given integer 'new_value' is stored in the area, and the old value
+ * is returned.
+ */
+PG_FUNCTION_INFO_V1(test_shmem_register);
+Datum
+test_shmem_register(PG_FUNCTION_ARGS)
+{
+	char	   *name = text_to_cstring(PG_GETARG_TEXT_PP(0));
+	int64		size = PG_GETARG_INT64(1);
+	int			new_value = PG_GETARG_INT32(2);
+	int			old_value;
+	int		   *attached = NULL;
+
+	ShmemStructOpts opts = {
+		.name = name,
+		.size = size,
+		.ptr = (void **) &attached,
+	};
+
+	ShmemCallbacks callbacks = {
+		.flags = SHMEM_CALLBACKS_ALLOW_AFTER_STARTUP,
+		.request_fn = test_shmem_after_startup_request,
+		.opaque_arg = &opts,
+	};
+
+	RegisterShmemCallbacks(&callbacks);
+	if (attached == NULL)
+		elog(ERROR, "could not attach to shared memory");
+
+	old_value = *attached;
+	*attached = new_value;
+
+	PG_RETURN_INT32(old_value);
+}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 大写宏：`ERROR`
- 大写宏：`PG_RETURN_INT32`
- 外部类型：`TestShmem`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-24be32a60f` → 本草稿移入 `cases/defect/auto-postgres-24be32a60f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
