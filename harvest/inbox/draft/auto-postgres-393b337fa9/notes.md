# auto-postgres-393b337fa9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#32dd742b075e999fa358570bc13778b93f8025d4](https://github.com/postgres/postgres/commit/32dd742b075e999fa358570bc13778b93f8025d4) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #32dd742b075e999fa358570bc13778b93f8025d4 (https://github.com/postgres/postgres/commit/32dd742b075e999fa358570bc13778b93f8025d4)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 184；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 32dd742b075e999fa358570bc13778b93f8025d4 pg_upgrade: Preserve template0 ICU rules :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -12,6 +12,7 @@
 #include <signal.h>
 
 #include "common/username.h"
+#include "lib/stringinfo.h"
 #include "pg_upgrade.h"
 
 LogOpts		log_opts;
@@ -175,13 +176,30 @@ prep_status_progress(const char *fmt, ...)
 static void
 pg_log_v(eLogType type, const char *fmt, va_list ap)
 {
-	char		message[QUERY_ALLOC];
+	int			save_errno = errno;
+	StringInfoData buf;
+	char	   *message;
 
 	/* No incoming message should end in newline; we add that here. */
 	Assert(fmt);
 	Assert(fmt[0] == '\0' || fmt[strlen(fmt) - 1] != '\n');
 
-	vsnprintf(message, sizeof(message), _(fmt), ap);
+	fmt = _(fmt);
+	initStringInfo(&buf);
+	for (;;)
+	{
+		va_list		ap2;
+		int			needed;
+
+		errno = save_errno;
+		va_copy(ap2, ap);
+		needed = appendStringInfoVA(&buf, fmt, ap2);
+		va_end(ap2);
+		if (needed == 0)
+			break;
+		enlargeStringInfo(&buf, needed);
+	}
+	message = buf.data;
 
 	/* PG_VERBOSE and PG_STATUS are only output in verbose mode */
 	/* fopen() on log_opts.internal might have failed, so check it */
@@ -251,6 +269,7 @@ pg_log_v(eLogType type, const char *fmt, va_list ap)
 
 			/* No default:, we want a warning for omitted cases */
 	}
+	pg_free(message);
 	fflush(stdout);
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_`
- 外部函数：`fflush`
- 外部函数：`fopen`
- 外部函数：`strlen`
- 外部函数：`vsnprintf`
- 大写宏：`PG_STATUS`
- 大写宏：`PG_VERBOSE`
- 大写宏：`QUERY_ALLOC`
- 外部类型：`LogOpts`
- 外部类型：`No`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-393b337fa9` → 本草稿移入 `cases/defect/auto-postgres-393b337fa9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
