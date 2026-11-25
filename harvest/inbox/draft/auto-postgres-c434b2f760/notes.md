# auto-postgres-c434b2f760

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#93e2b79f5f66ee305338bd956e70a96c9d114ac7](https://github.com/postgres/postgres/commit/93e2b79f5f66ee305338bd956e70a96c9d114ac7) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #93e2b79f5f66ee305338bd956e70a96c9d114ac7 (https://github.com/postgres/postgres/commit/93e2b79f5f66ee305338bd956e70a96c9d114ac7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1991；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 93e2b79f5f66ee305338bd956e70a96c9d114ac7 Avoid null pointer crash after pg_localtime()/pg_gmtime() failure. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1978,33 +1978,35 @@ timestamp2tm(Timestamp dt, int *tzp, struct pg_tm *tm, fsec_t *fsec, const char
 	{
 		struct pg_tm *tx = pg_localtime(&utime, attimezone);
 
-		tm->tm_year = tx->tm_year + 1900;
-		tm->tm_mon = tx->tm_mon + 1;
-		tm->tm_mday = tx->tm_mday;
-		tm->tm_hour = tx->tm_hour;
-		tm->tm_min = tx->tm_min;
-		tm->tm_sec = tx->tm_sec;
-		tm->tm_isdst = tx->tm_isdst;
-		tm->tm_gmtoff = tx->tm_gmtoff;
-		tm->tm_zone = tx->tm_zone;
-		*tzp = -tm->tm_gmtoff;
-		if (tzn != NULL)
-			*tzn = tm->tm_zone;
-	}
-	else
-	{
-		/*
-		 * When out of range of pg_time_t, treat as GMT
-		 */
-		*tzp = 0;
-		/* Mark this as *no* time zone available */
-		tm->tm_isdst = -1;
-		tm->tm_gmtoff = 0;
-		tm->tm_zone = NULL;
-		if (tzn != NULL)
-			*tzn = NULL;
+		/* Treat failure as out-of-range, though it could be ENOMEM */
+		if (tx)
+		{
+			tm->tm_year = tx->tm_year + 1900;
+			tm->tm_mon = tx->tm_mon + 1;
+			tm->tm_mday = tx->tm_mday;
+			tm->tm_hour = tx->tm_hour;
+			tm->tm_min = tx->tm_min;
+			tm->tm_sec = tx->tm_sec;
+			tm->tm_isdst = tx->tm_isdst;
+			tm->tm_gmtoff = tx->tm_gmtoff;
+			tm->tm_zone = tx->tm_zone;
+			*tzp = -tm->tm_gmtoff;
+			if (tzn != NULL)
+				*tzn = tm->tm_zone;
+			return 0;
+		}
 	}
 
+	/*
+	 * When out of range of pg_time_t, treat as GMT
+	 */
+	*tzp = 0;
+	/* Mark this as *no* time zone available */
+	tm->tm_isdst = -1;
+	tm->tm_gmtoff = 0;
+	tm->tm_zone = NULL;
+	if (tzn != NULL)
+		*tzn = NULL;
 	return 0;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pg_localtime`
- 大写宏：`GMT`
- 大写宏：`NULL`
- 外部类型：`Mark`
- 外部类型：`When`
- 外部类型：`pg_time_t`
- 外部类型：`pg_tm`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c434b2f760` → 本草稿移入 `cases/defect/auto-postgres-c434b2f760/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
