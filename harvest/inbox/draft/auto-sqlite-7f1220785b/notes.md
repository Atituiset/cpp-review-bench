# auto-sqlite-7f1220785b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#b4b5ba2afb750999741ebd658d926af2cca3fe64](https://github.com/sqlite/sqlite/commit/b4b5ba2afb750999741ebd658d926af2cca3fe64) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 95 |
| 编译错误数（gcc syntax-only） | 31（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b4b5ba2afb750999741ebd658d926af2cca3fe64 (https://github.com/sqlite/sqlite/commit/b4b5ba2afb750999741ebd658d926af2cca3fe64)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 5413；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b4b5ba2afb750999741ebd658d926af2cca3fe64 Improved incrblob recovery when the content of the row it is reading is :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5388,14 +5388,15 @@ int sqlite3BtreePayload(BtCursor *pCur, u32 offset, u32 amt, void *pBuf){
 ** This variant of sqlite3BtreePayload() works even if the cursor has not
 ** in the CURSOR_VALID state.  It is only used by the sqlite3_blob_read()
 ** interface.
+**
+** If the cursor has to be restored to its saved position, then SQLITE_MOVED
+** is returned and no data is read.  The content of the row might have changed
+** while the cursor position was saved, so the caller needs to recompute the
+** size and offset of the data it is reading before it tries again.  If the
+** row no longer exists, SQLITE_ABORT is returned.
 */
 #ifndef SQLITE_OMIT_INCRBLOB
-static SQLITE_NOINLINE int accessPayloadChecked(
-  BtCursor *pCur,
-  u32 offset,
-  u32 amt,
-  void *pBuf
-){
+static SQLITE_NOINLINE int accessPayloadChecked(BtCursor *pCur){
   int rc;
   if ( pCur->eState==CURSOR_INVALID ){
     return SQLITE_ABORT;
@@ -5407,20 +5408,15 @@ static SQLITE_NOINLINE int accessPayloadChecked(
     /* The row is gone, and the cursor was restored onto a neighbor. */
     return SQLITE_ABORT;
   }
-  return sqlite3BtreePayloadChecked(pCur, offset, amt, pBuf);
+  return SQLITE_MOVED;
 }
 int sqlite3BtreePayloadChecked(BtCursor *pCur, u32 offset, u32 amt, void *pBuf){
-  if( pCur->eState!=CURSOR_VALID ){
-    return accessPayloadChecked(pCur, offset, amt, pBuf);
-  }
-  assert( cursorOwnsBtShared(pCur) );
-  /* The size of the row might have changed.  Recompute it. */
-  if( pCur->info.nSize==0 ) getCellInfo(pCur);
-  if( (u64)offset+(u64)amt > (u64)pCur->info.nPayload ){
-    /* Size of the row has changed and the caller didn't notice. */
-    return SQLITE_ABORT;
+  if( pCur->eState==CURSOR_VALID ){
+    assert( cursorOwnsBtShared(pCur) );
+    return accessPayload(pCur, offset, amt, pBuf, 0);
+  }else{
+    return accessPayloadChecked(pCur);
   }
-  return accessPayload(pCur, offset, amt, pBuf, 0);
 }
 #endif /* SQLITE_OMIT_INCRBLOB */
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`accessPayloadChecked`
- 外部函数：`assert`
- 外部函数：`btreeParseCell`
- 外部函数：`buffer`
- 外部函数：`cursorHoldsMutex`
- 外部函数：`findCell`
- 外部函数：`get2byteAligned`
- 外部函数：`get4byte`
- 外部函数：`getCellInfo`
- 外部函数：`handle`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`page`
- 外部函数：`read`
- 外部函数：`sqlite3BeginBenignMalloc`
- 外部函数：`sqlite3EndBenignMalloc`
- 外部函数：`sqlite3FaultSim`
- 外部函数：`sqlite3MallocSize`
- 外部函数：`sqlite3OsRead`
- 外部函数：`sqlite3PagerDirectReadOk`
- 外部函数：`sqlite3PagerFile`
- 外部函数：`sqlite3PagerFilename`
- 外部函数：`sqlite3PagerGet`
- 外部函数：`sqlite3PagerGetData`
- 外部函数：`sqlite3PagerGetExtra`
- 外部函数：`sqlite3PagerPageRefcount`
- 外部函数：`sqlite3PagerUnref`
- 外部函数：`sqlite3PagerUnrefNotNull`
- 外部函数：`sqlite3PagerWrite`
- 外部函数：`sqlite3Realloc`
- 外部函数：`sqlite3ReportError`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`testcase`
- 外部函数：`to`
- 外部函数：`written`
- 外部函数：`xParseCell`
- 大写宏：`CORRUPT_DB`
- 大写宏：`CURSOR_INVALID`
- 大写宏：`CURSOR_VALID`
- 大写宏：`NEVER`
- 大写宏：`NULL`
- 大写宏：`OUT`
- 大写宏：`PAGER_GET_NOCONTENT`
- 大写宏：`PAGER_GET_READONLY`
- 大写宏：`PENDING_BYTE_PAGE`
- 大写宏：`PTRMAP_ISPAGE`
- 大写宏：`PTRMAP_OVERFLOW2`
- 大写宏：`SQLITE_ABORT`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_DIRECT_OVERFLOW_READ`
- 大写宏：`SQLITE_DONE`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_AUTOVACUUM`
- 大写宏：`SQLITE_OMIT_INCRBLOB`
- 大写宏：`WAL`
- 外部类型：`BTCF_ValidNKey`
- 外部类型：`BTCF_ValidOvfl`
- 外部类型：`Begin`
- 外部类型：`BtCursor`
- 外部类型：`BtShared`
- 外部类型：`Btree`
- 外部类型：`Bytes`
- 外部类型：`CellInfo`
- 外部类型：`Check`
- 外部类型：`Copy`
- 外部类型：`Current`
- 外部类型：`Cursor`
- 外部类型：`DbPage`
- 外部类型：`Fill`
- 外部类型：`First`
- 外部类型：`Guess`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`MemPage`
- 外部类型：`Need`
- 外部类型：`Next`
- 外部类型：`Number`
- 外部类型：`Overflow`
- 外部类型：`Page`
- 外部类型：`Pgno`
- 外部类型：`Pointer`
- 外部类型：`Read`
- 外部类型：`Recompute`
- 外部类型：`Return`
- 外部类型：`Sanity`
- 外部类型：`Size`
- 外部类型：`So`
- 外部类型：`Start`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Try`
- 外部类型：`Trying`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7f1220785b` → 本草稿移入 `cases/defect/auto-sqlite-7f1220785b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
