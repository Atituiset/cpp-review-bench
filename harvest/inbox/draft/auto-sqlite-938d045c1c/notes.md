# auto-sqlite-938d045c1c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#63f025a33152b96707c75335a206b76e90a77b06](https://github.com/sqlite/sqlite/commit/63f025a33152b96707c75335a206b76e90a77b06) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 235 |
| 编译错误数（gcc syntax-only） | 81（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #63f025a33152b96707c75335a206b76e90a77b06 (https://github.com/sqlite/sqlite/commit/63f025a33152b96707c75335a206b76e90a77b06)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 3604；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 63f025a33152b96707c75335a206b76e90a77b06 Do not allow "PRAGMA mmap_size=0" while still in use. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3601,8 +3601,10 @@ static void pagerFixMaplimit(Pager *pPager){
   if( isOpen(fd) && fd->pMethods->iVersion>=3 ){
     sqlite3_int64 sz;
     sz = pPager->szMmap;
-    pPager->bUseFetch = (sz>0);
-    setGetterMethod(pPager);
+    if( sz>0 || pPager->nMmapOut==0 ){
+      pPager->bUseFetch = (sz>0);
+      setGetterMethod(pPager);
+    }
     sqlite3OsFileControlHint(pPager->fd, SQLITE_FCNTL_MMAP_SIZE, &sz);
   }
 #endif
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`addToSavepointBitvecs`
- 外部函数：`assert`
- 外部函数：`assert_pager_state`
- 外部函数：`change`
- 外部函数：`freeSuperJournal`
- 外部函数：`hash`
- 外部函数：`headers`
- 外部函数：`int`
- 外部函数：`isOpen`
- 外部函数：`lock`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`only`
- 外部函数：`pagerFlushOnCommit`
- 外部函数：`pagerUnlockDb`
- 外部函数：`pager_delsuper`
- 外部函数：`pager_error`
- 外部函数：`pager_playback_one_page`
- 外部函数：`pager_reset`
- 外部函数：`pager_set_pagehash`
- 外部函数：`pager_truncate`
- 外部函数：`readSuperJournal`
- 外部函数：`releaseAllSavepoints`
- 外部函数：`setSectorSize`
- 外部函数：`sqlite3BeginBenignMalloc`
- 外部函数：`sqlite3BitvecDestroy`
- 外部函数：`sqlite3BitvecSet`
- 外部函数：`sqlite3EndBenignMalloc`
- 外部函数：`sqlite3JournalIsInMemory`
- 外部函数：`sqlite3MallocZero`
- 外部函数：`sqlite3OsAccess`
- 外部函数：`sqlite3OsClose`
- 外部函数：`sqlite3OsDelete`
- 外部函数：`sqlite3OsDeviceCharacteristics`
- 外部函数：`sqlite3OsFetch`
- 外部函数：`sqlite3OsFileControl`
- 外部函数：`sqlite3OsFileControlHint`
- 外部函数：`sqlite3OsFileSize`
- 外部函数：`sqlite3OsRead`
- 外部函数：`sqlite3OsSync`
- 外部函数：`sqlite3OsTruncate`
- 外部函数：`sqlite3OsUnfetch`
- 外部函数：`sqlite3PagerSavepoint`
- 外部函数：`sqlite3PagerSetPagesize`
- 外部函数：`sqlite3PagerSync`
- 外部函数：`sqlite3PagerUnrefNotNull`
- 外部函数：`sqlite3PcacheCleanAll`
- 外部函数：`sqlite3PcacheClearWritable`
- 外部函数：`sqlite3PcacheDrop`
- 外部函数：`sqlite3PcacheFetch`
- 外部函数：`sqlite3PcacheFetchFinish`
- 外部函数：`sqlite3PcacheFetchStress`
- 外部函数：`sqlite3PcacheIterateDirty`
- 外部函数：`sqlite3PcacheRefCount`
- 外部函数：`sqlite3PcacheRelease`
- 外部函数：`sqlite3PcacheTruncate`
- 外部函数：`sqlite3WalEndReadTransaction`
- 外部函数：`sqlite3WalEndWriteTransaction`
- 外部函数：`sqlite3WalExclusiveMode`
- 外部函数：`sqlite3WalFindFrame`
- 外部函数：`sqlite3WalReadFrame`
- 外部函数：`sqlite3_log`
- 外部函数：`state`
- 外部函数：`testcase`
- 外部函数：`void`
- 外部函数：`xFetch`
- 外部函数：`xOpen`
- 外部函数：`zeroJournalHdr`
- 大写宏：`ACQUIRE_READONLY`
- 大写宏：`BEGIN`
- 大写宏：`EIGHT_BYTE_ALIGNMENT`
- 大写宏：`ERROR`
- 大写宏：`EXCLUSIVE`
- 大写宏：`EXCLUSIVE_LOCK`
- 大写宏：`FCNTL_SIZE_HINT`
- 大写宏：`FETCH`
- 大写宏：`IOTRACE`
- 大写宏：`JOURNAL_HDR_SZ`
- 大写宏：`JOURNAL_PG_SZ`
- 大写宏：`MEMORY`
- 大写宏：`NOTREACHED`
- 大写宏：`NO_LOCK`
- 大写宏：`OFF`
- 大写宏：`OPEN`
- 大写宏：`OUT`
- 大写宏：`PAGER_GET_NOCONTENT`
- 大写宏：`PAGER_GET_READONLY`
- 大写宏：`PAGER_GET_XXX`
- 大写宏：`PAGER_JOURNALMODE_`
- 大写宏：`PAGER_JOURNALMODE_DELETE`
- 大写宏：`PAGER_JOURNALMODE_MEMORY`
- 大写宏：`PAGER_JOURNALMODE_OFF`
- 大写宏：`PAGER_JOURNALMODE_PERSIST`
- 大写宏：`PAGER_JOURNALMODE_TRUNCATE`
- 大写宏：`PAGER_JOURNALMODE_WAL`
- 大写宏：`PAGER_NONE`
- 大写宏：`PAGER_SJ_PGNO`
- 大写宏：`PENDING_LOCK`
- 大写宏：`PGHDR_MMAP`
- 大写宏：`PGIN`
- 大写宏：`READER`
- 大写宏：`RESERVED`
- 大写宏：`RESERVED_LOCK`
- 大写宏：`ROLLBACK`
- 大写宏：`SAVEPOINT_ROLLBACK`
- 大写宏：`SHARED_LOCK`
- 大写宏：`SQLITE_ABORT`
- 大写宏：`SQLITE_ACCESS_EXISTS`
- 大写宏：`SQLITE_CANTOPEN`
- 大写宏：`SQLITE_CHECK_PAGES`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_DONE`
- 大写宏：`SQLITE_ENABLE_SETLK_TIMEOUT`
- 大写宏：`SQLITE_FCNTL_COMMIT_PHASETWO`
- 大写宏：`SQLITE_FCNTL_DB_UNCHANGED`
- 大写宏：`SQLITE_FCNTL_MMAP_SIZE`
- 大写宏：`SQLITE_FULL`
- 大写宏：`SQLITE_IOCAP_BATCH_ATOMIC`
- 大写宏：`SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN`
- 大写宏：`SQLITE_IOERR`
- 大写宏：`SQLITE_IOERR_NOMEM`
- 大写宏：`SQLITE_IOERR_SHORT_READ`
- 大写宏：`SQLITE_MAX_MMAP_SIZE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_NOTFOUND`
- 大写宏：`SQLITE_NOTICE_RECOVER_ROLLBACK`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_WAL`
- 大写宏：`SQLITE_PTR_TO_INT`
- 大写宏：`SQLITE_TEST`
- 大写宏：`SYNC_FULL`
- 大写宏：`SYNC_NORMAL`
- 大写宏：`TESTONLY`
- 大写宏：`UNUSED_PARAMETER`
- 大写宏：`VFS`
- 大写宏：`WAL`
- 大写宏：`WRITER_LOCKED`
- 大写宏：`ZERO`
- 外部类型：`Abort`
- 外部类型：`Acquired`
- 外部类型：`Add`
- 外部类型：`Adding`
- 外部类型：`After`
- 外部类型：`Also`
- 外部类型：`And`
- 外部类型：`Any`
- 外部类型：`Array`
- 外部类型：`Assumed`
- 外部类型：`At`
- 外部类型：`Bitvec`
- 外部类型：`Boolean`
- 外部类型：`But`
- 外部类型：`Byte`
- 外部类型：`Bytes`
- 外部类型：`Call`
- 外部类型：`Changes`
- 外部类型：`Check`
- 外部类型：`Class`
- 外部类型：`Compute`
- 外部类型：`Context`
- 外部类型：`Copy`
- 外部类型：`Current`
- 外部类型：`Database`
- 外部类型：`DbPage`
- 外部类型：`Desired`
- 外部类型：`Do`
- 外部类型：`Drop`
- 外部类型：`End`
- 外部类型：`Error`
- 外部类型：`Failure`
- 外部类型：`Figure`
- 外部类型：`File`
- 外部类型：`Finalize`
- 外部类型：`Flags`
- 外部类型：`Following`
- 外部类型：`For`
- 外部类型：`Frame`
- 外部类型：`From`
- 外部类型：`Function`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`InJournal`
- 外部类型：`Instead`
- 外部类型：`It`
- 外部类型：`Its`
- 外部类型：`List`
- 外部类型：`Loop`
- 外部类型：`Make`
- 外部类型：`Maximum`
- 外部类型：`Memory`
- 外部类型：`Move`
- 外部类型：`Name`
- 外部类型：`Never`
- 外部类型：`Nevertheless`
- 外部类型：`Now`
- 外部类型：`Number`
- 外部类型：`One`
- 外部类型：`Optimization`
- 外部类型：`Otherwise`
- 外部类型：`PCache`
- 外部类型：`Page`
- 外部类型：`Pager`
- 外部类型：`PagerSavepoint`
- 外部类型：`Pages`
- 外部类型：`Perhaps`
- 外部类型：`PgHdr`
- 外部类型：`Pgno`
- 外部类型：`Pointer`
- 外部类型：`Quasi`
- 外部类型：`Read`
- 外部类型：`Records`
- 外部类型：`Regardless`
- 外部类型：`Reset`
- 外部类型：`Result`
- 外部类型：`Return`
- 外部类型：`Routine`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`Size`
- 外部类型：`So`
- 外部类型：`Special`
- 外部类型：`State`
- 外部类型：`Super`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Total`
- 外部类型：`True`
- 外部类型：`Unsigned`
- 外部类型：`Use`
- 外部类型：`Value`
- 外部类型：`Wal`
- 外部类型：`Was`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Write`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-938d045c1c` → 本草稿移入 `cases/defect/auto-sqlite-938d045c1c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
