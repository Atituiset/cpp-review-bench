# auto-postgres-ea22c26ffd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#61a372b795ddec08c15518096fe1f3e9e9973096](https://github.com/postgres/postgres/commit/61a372b795ddec08c15518096fe1f3e9e9973096) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 47 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #61a372b795ddec08c15518096fe1f3e9e9973096 (https://github.com/postgres/postgres/commit/61a372b795ddec08c15518096fe1f3e9e9973096)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 2081；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 61a372b795ddec08c15518096fe1f3e9e9973096 Fix error handling in pg_flush_data() and FilePrefetch() :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -686,6 +686,9 @@ pg_flush_data(int fd, pgoff_t offset, pgoff_t nbytes)
 
 		if (rc != 0)
 		{
+			/* update errno for %m et al */
+			errno = rc;
+
 			/* don't error out, this is just a performance optimization */
 			ereport(WARNING,
 					(errcode_for_file_access(),
@@ -2078,7 +2081,7 @@ FilePrefetch(File file, pgoff_t offset, pgoff_t amount, uint32 wait_event_info)
 
 		returnCode = FileAccess(file);
 		if (returnCode < 0)
-			return returnCode;
+			return errno;
 
 retry:
 		pgstat_report_wait_start(wait_event_info);
@@ -2102,7 +2105,7 @@ FilePrefetch(File file, pgoff_t offset, pgoff_t amount, uint32 wait_event_info)
 
 		returnCode = FileAccess(file);
 		if (returnCode < 0)
-			return returnCode;
+			return errno;
 
 		ra.ra_offset = offset;
 		ra.ra_count = amount;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`close`
- 外部函数：`d`
- 外部函数：`data_sync_elevel`
- 外部函数：`elog`
- 外部函数：`errcode`
- 外部函数：`errcode_for_file_access`
- 外部函数：`errmsg`
- 外部函数：`fcntl`
- 外部函数：`file`
- 外部函数：`open`
- 外部函数：`pgaio_closing_fd`
- 外部函数：`pgstat_report_wait_start`
- 外部函数：`snprintf`
- 外部函数：`strlen`
- 大写宏：`EMFILE`
- 大写宏：`ENFILE`
- 大写宏：`ERRCODE_INSUFFICIENT_RESOURCES`
- 大写宏：`FD_TEMP_FILE_LIMIT`
- 大写宏：`F_NOCACHE`
- 大写宏：`LEAST`
- 大写宏：`LOG`
- 大写宏：`LRU`
- 大写宏：`MOST`
- 大写宏：`NULL`
- 大写宏：`PG_O_DIRECT`
- 大写宏：`PG_O_DIRECT_USE_F_NOCACHE`
- 大写宏：`VFD`
- 大写宏：`WARNING`
- 外部类型：`Close`
- 外部类型：`Delete`
- 外部类型：`FDs`
- 外部类型：`File`
- 外部类型：`FileAccess`
- 外部类型：`If`
- 外部类型：`Insert`
- 外部类型：`Is`
- 外部类型：`Lru`
- 外部类型：`LruDelete`
- 外部类型：`LruInsert`
- 外部类型：`Opened`
- 外部类型：`ReleaseLruFile`
- 外部类型：`ResourceOwner`
- 外部类型：`So`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Vfd`
- 外部类型：`VfdCache`
- 外部类型：`We`
- 外部类型：`mode_t`
- 外部类型：`pgoff_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ea22c26ffd` → 本草稿移入 `cases/defect/auto-postgres-ea22c26ffd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
