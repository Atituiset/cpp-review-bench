# auto-sqlite-2a3d1c7369

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#076ab6c28636bb73ff84c41d3ddbb59c78d46a7c](https://github.com/sqlite/sqlite/commit/076ab6c28636bb73ff84c41d3ddbb59c78d46a7c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 86 |
| 编译错误数（gcc syntax-only） | 50（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #076ab6c28636bb73ff84c41d3ddbb59c78d46a7c (https://github.com/sqlite/sqlite/commit/076ab6c28636bb73ff84c41d3ddbb59c78d46a7c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 9522；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 076ab6c28636bb73ff84c41d3ddbb59c78d46a7c Prevent application-defined functions that invoke DDL statement or :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -9492,6 +9492,7 @@ int sqlite3BtreeInsert(
     testcase( pCur->eState==CURSOR_FAULT );
     rc = moveToRoot(pCur);
     if( rc && rc!=SQLITE_EMPTY ) return rc;
+    loc = 0;  /* Indicate the the cursor is not in a known position */
   }
 
   assert( cursorOwnsBtShared(pCur) );
@@ -9515,11 +9516,13 @@ int sqlite3BtreeInsert(
       invalidateIncrblobCursors(p, pCur->pgnoRoot, pX->nKey, 0);
     }
 
-    /* If BTREE_SAVEPOSITION is set, the cursor must already be pointing
-    ** to a row with the same key as the new entry being inserted.
+    /* If BTREE_SAVEPOSITION is set and seekResult is non-zero and
+    ** if we didn't have to reposition the cursor initially, then
+    ** the cursor must already be pointing to a row with the same
+    ** key as the new entry being inserted.  Verify that this is so.
     */
 #ifdef SQLITE_DEBUG
-    if( flags & BTREE_SAVEPOSITION ){
+    if( (flags & BTREE_SAVEPOSITION)!=0 && loc!=0 ){
       assert( pCur->curFlags & BTCF_ValidNKey );
       assert( pX->nKey==pCur->info.nKey );
       assert( loc==0 );
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`btreeParseCellAdjustSizeForOverflow`
- 外部函数：`called`
- 外部函数：`cells`
- 外部函数：`cursorHoldsMutex`
- 外部函数：`get2byte`
- 外部函数：`get2byteAligned`
- 外部函数：`get4byte`
- 外部函数：`getVarint32`
- 外部函数：`sqlite3BeginBenignMalloc`
- 外部函数：`sqlite3BtreeClearCursor`
- 外部函数：`sqlite3EndBenignMalloc`
- 外部函数：`sqlite3GetVarint`
- 外部函数：`sqlite3PagerFilename`
- 外部函数：`sqlite3PagerGet`
- 外部函数：`sqlite3PagerGetData`
- 外部函数：`sqlite3PagerGetExtra`
- 外部函数：`sqlite3PagerPagenumber`
- 外部函数：`sqlite3PagerUnrefNotNull`
- 外部函数：`sqlite3ReportError`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`testcase`
- 外部函数：`the`
- 外部函数：`xCellSize`
- 外部函数：`xParseCell`
- 大写宏：`BTCURSOR_MAX_DEPTH`
- 大写宏：`BTREE_SAVEPOSITION`
- 大写宏：`BT_MAX_LOCAL`
- 大写宏：`CORRUPT_DB`
- 大写宏：`CURSOR_FAULT`
- 大写宏：`CURSOR_INVALID`
- 大写宏：`CURSOR_REQUIRESEEK`
- 大写宏：`CURSOR_VALID`
- 大写宏：`EVIDENCE`
- 大写宏：`MX_CELL`
- 大写宏：`NULL`
- 大写宏：`PTF_INTKEY`
- 大写宏：`PTF_LEAF`
- 大写宏：`PTF_LEAFDATA`
- 大写宏：`PTF_ZERODATA`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_CORRUPT_PGNO`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_EMPTY`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_OK`
- 大写宏：`UNUSED_PARAMETER`
- 外部类型：`Address`
- 外部类型：`BTCF_AtLast`
- 外部类型：`BTCF_ValidNKey`
- 外部类型：`BTCF_ValidOvfl`
- 外部类型：`BtCursor`
- 外部类型：`BtShared`
- 外部类型：`But`
- 外部类型：`CellInfo`
- 外部类型：`DbPage`
- 外部类型：`Earlier`
- 外部类型：`End`
- 外部类型：`Equal`
- 外部类型：`Extracted`
- 外部类型：`Fill`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Indicate`
- 外部类型：`Key`
- 外部类型：`Last`
- 外部类型：`Maximum`
- 外部类型：`MemPage`
- 外部类型：`No`
- 外部类型：`Number`
- 外部类型：`Otherwise`
- 外部类型：`Page`
- 外部类型：`Pgno`
- 外部类型：`Pointer`
- 外部类型：`SQLITE_CellSizeCk`
- 外部类型：`SQLite`
- 外部类型：`Size`
- 外部类型：`Start`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`True`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2a3d1c7369` → 本草稿移入 `cases/defect/auto-sqlite-2a3d1c7369/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
