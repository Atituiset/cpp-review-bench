# auto-postgres-9e1ecc011e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#51d7bf5edcaf4f8a95bb10f96b7af76d00a2f83f](https://github.com/postgres/postgres/commit/51d7bf5edcaf4f8a95bb10f96b7af76d00a2f83f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 84 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #51d7bf5edcaf4f8a95bb10f96b7af76d00a2f83f (https://github.com/postgres/postgres/commit/51d7bf5edcaf4f8a95bb10f96b7af76d00a2f83f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 1706；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 51d7bf5edcaf4f8a95bb10f96b7af76d00a2f83f Avoid truncated or zero-padded copies of replaced files in backups :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -93,7 +93,7 @@ static int64 sendDir(bbsink *sink, const char *path, int basepathlen, bool sizeo
 					 backup_manifest_info *manifest, Oid spcoid,
 					 IncrementalBackupInfo *ib);
 static bool sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
-					 struct stat *statbuf, bool missing_ok,
+					 const struct stat *statbuf, bool missing_ok,
 					 Oid dboid, Oid spcoid, RelFileNumber relfilenumber,
 					 unsigned segno,
 					 backup_manifest_info *manifest,
@@ -1575,12 +1575,15 @@ sendDir(bbsink *sink, const char *path, int basepathlen, bool sizeonly,
  * If the whole file is to be sent, then incremental_blocks should be NULL,
  * and num_incremental_blocks can have any value, as it will be ignored.
  *
+ * For non-relation files, the stat data of the opened file is used, not
+ * 'statbuf'.
+ *
  * Returns true if the file was successfully sent, false if 'missing_ok',
  * and the file did not exist.
  */
 static bool
 sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
-		 struct stat *statbuf, bool missing_ok, Oid dboid, Oid spcoid,
+		 const struct stat *statbuf, bool missing_ok, Oid dboid, Oid spcoid,
 		 RelFileNumber relfilenumber, unsigned segno,
 		 backup_manifest_info *manifest, unsigned num_incremental_blocks,
 		 BlockNumber *incremental_blocks, unsigned truncation_block_length)
@@ -1593,6 +1596,7 @@ sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
 	bool		verify_checksum = false;
 	pg_checksum_context checksum_ctx;
 	int			ibindex = 0;
+	struct stat filestat;
 
 	if (pg_checksum_init(&checksum_ctx, manifest->checksum_type) < 0)
 		elog(ERROR, "could not initialize checksum of file \"%s\"",
@@ -1608,7 +1612,18 @@ sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
 				 errmsg("could not open file \"%s\": %m", readfilename)));
 	}
 
-	_tarWriteHeader(sink, tarfilename, NULL, statbuf, false);
+	/*
+	 * Non-relation files may be replaced by a rename(), making irrelevant the
+	 * caller's stat data.  Reuse fstat() on the opened file.
+	 */
+	filestat = *statbuf;
+
+	if (!RelFileNumberIsValid(relfilenumber) && fstat(fd, &filestat) != 0)
+		ereport(ERROR,
+				(errcode_for_file_access(),
+				 errmsg("could not stat file \"%s\": %m", readfilename)));
+
+	_tarWriteHeader(sink, tarfilename, NULL, &filestat, false);
 
 	/*
 	 * Checksums are verified in multiples of BLCKSZ, so the buffer length
@@ -1697,13 +1712,13 @@ sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
 		 */
 		if (incremental_blocks == NULL)
 		{
-			size_t		remaining = statbuf->st_size - bytes_done;
+			size_t		remaining = filestat.st_size - bytes_done;
 
 			/*
 			 * If we've read the required number of bytes, then it's time to
 			 * stop.
 			 */
-			if (bytes_done >= statbuf->st_size)
+			if (bytes_done >= filestat.st_size)
 				break;
 
 			/*
@@ -1798,9 +1813,9 @@ sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
 	}
 
 	/* If the file was truncated while we were sending it, pad it with zeros */
-	while (bytes_done < statbuf->st_size)
+	while (bytes_done < filestat.st_size)
 	{
-		size_t		remaining = statbuf->st_size - bytes_done;
+		size_t		remaining = filestat.st_size - bytes_done;
 		size_t		nbytes = Min(sink->bbs_buffer_length, remaining);
 
 		MemSet(sink->bbs_buffer, 0, nbytes);
@@ -1835,8 +1850,8 @@ sendFile(bbsink *sink, const char *readfilename, const char *tarfilename,
 
 	total_checksum_failures += checksum_failures;
 
-	AddFileToBackupManifest(manifest, spcoid, tarfilename, statbuf->st_size,
-							(pg_time_t) statbuf->st_mtime, &checksum_ctx);
+	AddFileToBackupManifest(manifest, spcoid, tarfilename, filestat.st_size,
+							(pg_time_t) filestat.st_mtime, &checksum_ctx);
 
 	return true;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_tarWriteHeader`
- 外部函数：`backup_checksums_verifiable`
- 外部函数：`bbsink_archive_contents`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode_for_file_access`
- 外部函数：`errmsg`
- 外部函数：`memset`
- 外部函数：`pg_checksum_init`
- 外部函数：`pg_checksum_update`
- 外部函数：`push_to_sink`
- 外部函数：`read_file_data_into_buffer`
- 大写宏：`BLCKSZ`
- 大写宏：`ENOENT`
- 大写宏：`ERROR`
- 大写宏：`INCREMENTAL_MAGIC`
- 大写宏：`NULL`
- 大写宏：`O_RDONLY`
- 大写宏：`PG_BINARY`
- 大写宏：`RELSEG_SIZE`
- 大写宏：`WAL`
- 外部类型：`Add`
- 外部类型：`BlockNumber`
- 外部类型：`Checksums`
- 外部类型：`Determine`
- 外部类型：`Emit`
- 外部类型：`Flush`
- 外部类型：`If`
- 外部类型：`IncrementalBackupInfo`
- 外部类型：`It`
- 外部类型：`Loop`
- 外部类型：`Oid`
- 外部类型：`Read`
- 外部类型：`RelFileNumber`
- 外部类型：`Returns`
- 外部类型：`The`
- 外部类型：`Ultimately`
- 外部类型：`Update`
- 外部类型：`Verify`
- 外部类型：`off_t`
- 外部类型：`pgoff_t`
- 外部类型：`size_t`
- 外部类型：`stat`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-9e1ecc011e` → 本草稿移入 `cases/defect/auto-postgres-9e1ecc011e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
