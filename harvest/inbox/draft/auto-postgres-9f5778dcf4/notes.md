# auto-postgres-9f5778dcf4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#3c5d9d914fa5b8fb3f371dd97bdece032ca3598d](https://github.com/postgres/postgres/commit/3c5d9d914fa5b8fb3f371dd97bdece032ca3598d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 46 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3c5d9d914fa5b8fb3f371dd97bdece032ca3598d (https://github.com/postgres/postgres/commit/3c5d9d914fa5b8fb3f371dd97bdece032ca3598d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 3553；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3c5d9d914fa5b8fb3f371dd97bdece032ca3598d Don't emit garbage "\[un]restrict (null)" commands in pg_dump. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3550,14 +3550,16 @@ _reconnectToDB(ArchiveHandle *AH, const char *dbname)
 		 * Anything added between this line and the following \restrict must
 		 * be careful to avoid any possible meta-command injection vectors.
 		 */
-		ahprintf(AH, "\\unrestrict %s\n", ropt->restrict_key);
+		if (ropt->restrict_key)
+			ahprintf(AH, "\\unrestrict %s\n", ropt->restrict_key);
 
 		initPQExpBuffer(&connectbuf);
 		appendPsqlMetaConnect(&connectbuf, dbname);
 		ahprintf(AH, "%s", connectbuf.data);
 		termPQExpBuffer(&connectbuf);
 
-		ahprintf(AH, "\\restrict %s\n\n", ropt->restrict_key);
+		if (ropt->restrict_key)
+			ahprintf(AH, "\\restrict %s\n\n", ropt->restrict_key);
 	}
 
 	/*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendByteaLiteralAHX`
- 外部函数：`appendPsqlMetaConnect`
- 外部函数：`createPQExpBuffer`
- 外部函数：`data`
- 外部函数：`destroyPQExpBuffer`
- 外部函数：`exit_nicely`
- 外部函数：`initPQExpBuffer`
- 外部函数：`is_cancel_in_progress`
- 外部函数：`lo_write`
- 外部函数：`lowrite`
- 外部函数：`memcpy`
- 外部函数：`ngettext`
- 外部函数：`pg_free`
- 外部函数：`pg_log_debug`
- 外部函数：`pg_log_generic_v`
- 外部函数：`pg_log_info`
- 外部函数：`pg_malloc`
- 外部函数：`pvsnprintf`
- 外部函数：`termPQExpBuffer`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`write_func`
- 大写宏：`CFH`
- 大写宏：`FINALIZING`
- 大写宏：`INITIALIZING`
- 大写宏：`NULL`
- 大写宏：`PG_LOG_ERROR`
- 大写宏：`PG_LOG_PRIMARY`
- 大写宏：`PROCESSING`
- 大写宏：`SELECT`
- 大写宏：`STAGE_FINALIZING`
- 大写宏：`STAGE_INITIALIZING`
- 大写宏：`STAGE_NONE`
- 大写宏：`STAGE_PROCESSING`
- 大写宏：`TOC`
- 大写宏：`WRITE_ERROR_EXIT`
- 外部类型：`Allocate`
- 外部类型：`Anything`
- 外部类型：`ArchiveHandle`
- 外部类型：`CompressFileHandle`
- 外部类型：`CustomOutPtr`
- 外部类型：`Do`
- 外部类型：`Hack`
- 外部类型：`If`
- 外部类型：`PQExpBuffer`
- 外部类型：`Release`
- 外部类型：`RestoreOptions`
- 外部类型：`Stay`
- 外部类型：`Try`
- 外部类型：`We`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-9f5778dcf4` → 本草稿移入 `cases/defect/auto-postgres-9f5778dcf4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
