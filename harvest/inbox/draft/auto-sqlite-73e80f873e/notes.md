# auto-sqlite-73e80f873e

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
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b4b5ba2afb750999741ebd658d926af2cca3fe64 (https://github.com/sqlite/sqlite/commit/b4b5ba2afb750999741ebd658d926af2cca3fe64)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 455；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b4b5ba2afb750999741ebd658d926af2cca3fe64 Improved incrblob recovery when the content of the row it is reading is :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -26,6 +26,7 @@ struct Incrblob {
   int nByte;              /* Size of open blob, in bytes */
   int iOffset;            /* Byte offset of blob in cursor data */
   u16 iCol;               /* Table column this handle is open on */
+  u8 bRetry;              /* Doing a blobReadWrite() retry */
   BtCursor *pCsr;         /* Cursor pointing at blob row */
   sqlite3_stmt *pStmt;    /* Statement holding cursor open */
   sqlite3 *db;            /* The associated database */
@@ -375,6 +376,65 @@ int sqlite3_blob_close(sqlite3_blob *pBlob){
   return rc;
 }
 
+/* Forward reference */
+static int blobReadWrite(
+  sqlite3_blob *pBlob, 
+  void *z, 
+  int n, 
+  int iOffset, 
+  int (*xCall)(BtCursor*, u32, u32, void*)
+);
+
+/*
+** The b-tree cursor for blob handle p was restored to its saved position by
+** sqlite3BtreePayloadChecked(), which then returned SQLITE_MOVED.  The
+** position is saved, for example, by a ROLLBACK, and that might have changed
+** the content of the row.  So the iOffset and nByte values computed when the
+** handle was opened or last moved cannot be trusted.  A ROLLBACK can change
+** the size of a column that precedes the blob, which moves the blob within
+** the row.
+**
+** Recompute iOffset and nByte in the same way that sqlite3_blob_reopen()
+** does.  Then retry the blobReadWrite().
+*/
+static SQLITE_NOINLINE int blobReadWriteRetry(
+  Incrblob *p,
+  void *z, 
+  int n, 
+  int iOffset, 
+  int (*xCall)(BtCursor*, u32, u32, void*)
+){
+  int rc;
+  sqlite3_int64 iRow;
+  char *zErr = 0;
+  sqlite3 *db = p->db;
+
+  sqlite3BtreeEnterCursor(p->pCsr);
+  iRow = sqlite3BtreeIntegerKey(p->pCsr);
+  sqlite3BtreeLeaveCursor(p->pCsr);
+  ((Vdbe*)p->pStmt)->rc = SQLITE_OK;
+  if( NEVER(p->bRetry) || sqlite3FaultSim(210) ){
+    /* It should not be possible for blobReadWrite() and blobReadWriteRetry()
+    ** to get into a loop, but we check anyway, for safety. */
+    rc = SQLITE_ABORT;
+    sqlite3VdbeFinalize((Vdbe*)p->pStmt);
+    p->pStmt = 0;
+  }else{
+    rc = blobSeekToRow(p, iRow, &zErr);
+  }
+  sqlite3DbFree(db, zErr);
+  assert( rc==SQLITE_OK || p->pStmt==0 );
+  if( rc==SQLITE_OK ){
+    p->bRetry = 1;
+    rc = blobReadWrite((sqlite3_blob*)p,z,n,iOffset,xCall);
+    p->bRetry = 0;
+  }else{
+    rc = sqlite3ApiExit(db, rc);
+  }
+  sqlite3_mutex_leave(db->mutex);
+  return rc;
+}
+
 /*
 ** Perform a read or write operation on a blob
 */
@@ -452,11 +512,21 @@ static int blobReadWrite(
 #endif
 
     sqlite3BtreeLeaveCursor(p->pCsr);
-    if( rc==SQLITE_ABORT ){
-      sqlite3VdbeFinalize(v);
-      p->pStmt = 0;
+    if( rc==SQLITE_OK ){
+      v->rc = SQLITE_OK;
     }else{
-      v->rc = rc;
+      if( rc==SQLITE_MOVED ){
+        /* The cursor was restored and no data was read.  Refresh the
+        ** handle and try the read again. */
+        return blobReadWriteRetry(p,z,n,iOffset,xCall);
+      }
+      if( rc==SQLITE_ABORT ){
+        assert( p->pStmt );
+        sqlite3VdbeFinalize(v);
+        p->pStmt = 0;
+      }else{
+        v->rc = rc;
+      }
     }
   }
   sqlite3Error(db, rc);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3BtreeLeaveCursor`
- 外部函数：`sqlite3Error`
- 外部函数：`sqlite3VdbeFinalize`
- 大写宏：`SQLITE_ABORT`
- 外部类型：`BtCursor`
- 外部类型：`Byte`
- 外部类型：`Cursor`
- 外部类型：`Perform`
- 外部类型：`Size`
- 外部类型：`Statement`
- 外部类型：`Table`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-73e80f873e` → 本草稿移入 `cases/defect/auto-sqlite-73e80f873e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
