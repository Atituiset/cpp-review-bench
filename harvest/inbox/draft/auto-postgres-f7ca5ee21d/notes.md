# auto-postgres-f7ca5ee21d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#c97931c16d89c848696d2110a0f4831e4d6d1b69](https://github.com/postgres/postgres/commit/c97931c16d89c848696d2110a0f4831e4d6d1b69) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c97931c16d89c848696d2110a0f4831e4d6d1b69 (https://github.com/postgres/postgres/commit/c97931c16d89c848696d2110a0f4831e4d6d1b69)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 350；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c97931c16d89c848696d2110a0f4831e4d6d1b69 Work around failure with SPARC in base backup code :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -347,7 +347,7 @@ perform_base_backup(basebackup_options *opt, bbsink *sink,
 				char	   *backup_label;
 				ControlFileData *controlData;
 				bool		crc_ok;
-				char		controlFile[PG_CONTROL_FILE_SIZE];
+				char	   *controlFile;
 
 				bbsink_begin_archive(sink, "base.tar");
 
@@ -379,13 +379,14 @@ perform_base_backup(basebackup_options *opt, bbsink *sink,
 							(errmsg("calculated CRC checksum does not match value stored in file")));
 
 				/* Send the full file, zero-padded as it is on disk */
-				memset(controlFile, 0, PG_CONTROL_FILE_SIZE);
+				controlFile = palloc0(PG_CONTROL_FILE_SIZE);
 				memcpy(controlFile, controlData, sizeof(ControlFileData));
 				pfree(controlData);
 
 				sendFileWithContent(sink, XLOG_CONTROL_FILE,
 									controlFile, PG_CONTROL_FILE_SIZE,
 									&manifest);
+				pfree(controlFile);
 			}
 			else
 			{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bbsink_archive_contents`
- 外部函数：`bbsink_begin_archive`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`getegid`
- 外部函数：`geteuid`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`pfree`
- 外部函数：`pg_checksum_init`
- 外部函数：`pg_checksum_update`
- 外部函数：`strlen`
- 外部函数：`tarCreateHeader`
- 外部函数：`tarPaddingBytesRequired`
- 外部函数：`time`
- 大写宏：`BLCKSZ`
- 大写宏：`CRC`
- 大写宏：`ERRCODE_PROGRAM_LIMIT_EXCEEDED`
- 大写宏：`ERROR`
- 大写宏：`NULL`
- 大写宏：`PG_CONTROL_FILE_SIZE`
- 大写宏：`TAR_BLOCK_SIZE`
- 大写宏：`TAR_NAME_TOO_LONG`
- 大写宏：`TAR_OK`
- 大写宏：`TAR_SYMLINK_TOO_LONG`
- 大写宏：`WIN32`
- 大写宏：`XLOG_CONTROL_FILE`
- 外部类型：`As`
- 外部类型：`Construct`
- 外部类型：`ControlFileData`
- 外部类型：`InvalidOid`
- 外部类型：`Send`
- 外部类型：`Since`
- 外部类型：`We`
- 外部类型：`Windows`
- 外部类型：`for`
- 外部类型：`pg_time_t`
- 外部类型：`size_t`
- 外部类型：`stat`
- 外部类型：`tarError`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f7ca5ee21d` → 本草稿移入 `cases/defect/auto-postgres-f7ca5ee21d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
