# auto-sqlite-60d1092f14

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#2a9537b719e4e5fc3d5b5ef1bcd6fff8a7387df2](https://github.com/sqlite/sqlite/commit/2a9537b719e4e5fc3d5b5ef1bcd6fff8a7387df2) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2a9537b719e4e5fc3d5b5ef1bcd6fff8a7387df2 (https://github.com/sqlite/sqlite/commit/2a9537b719e4e5fc3d5b5ef1bcd6fff8a7387df2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1103；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2a9537b719e4e5fc3d5b5ef1bcd6fff8a7387df2 Ensure that all automatic-indexes constructed on WITHOUT ROWID tables always include all PK columns, sorted using either :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1011,6 +1011,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
   WhereLoop *pLoop;           /* The Loop object */
   char *zNotUsed;             /* Extra space on the end of pIdx */
   Bitmask idxCols;            /* Bitmap of columns used for indexing */
+  Bitmask pkCols;             /* PK cols whose key collation keeps them unique */
   Bitmask extraCols;          /* Bitmap of additional columns */
   u8 sentWarning = 0;         /* True if a warning has been issued */
   u8 useBloomFilter = 1;      /* True to also add a Bloom filter */
@@ -1039,6 +1040,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
   pWCEnd = &pWC->a[pWC->nTerm];
   pLoop = pLevel->pWLoop;
   idxCols = 0;
+  pkCols = 0;
   for(pTerm=pWC->a; pTerm<pWCEnd; pTerm++){
     Expr *pExpr = pTerm->pExpr;
     /* Make the automatic index a partial index if there are terms in the
@@ -1070,6 +1072,25 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
         }
         pLoop->aLTerm[nKeyCol++] = pTerm;
         idxCols |= cMask;
+
+        /* For a WITHOUT ROWID table, all PRIMARY KEY columns must be
+        ** present in the automatic index, sorted with either BINARY or
+        ** the same collation used by the PK index itself, to guarantee each
+        ** entry is unique. If the current field matches that criteria,
+        ** set the corresponding bit in pkCols.  */
+        if( !HasRowid(pTable) && iCol<BMS-1
+         && (pTable->aCol[iCol].colFlags & COLFLAG_PRIMKEY)
+        ){
+          Index *pPk = sqlite3PrimaryKeyIndex(pTable);
+          CollSeq *pX = sqlite3ExprCompareCollSeq(pParse, pExpr);
+          int iPk = sqlite3TableColumnToIndex(pPk, iCol);
+          assert( iPk>=0 && iPk<pPk->nKeyCol );
+          if( sqlite3IsBinary(pX)
+           || sqlite3StrICmp(pPk->azColl[iPk], pX->zName)==0
+          ){
+            pkCols |= cMask;
+          }
+        }
       }
     }
   }
@@ -1093,14 +1114,16 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
   }
   if( !HasRowid(pTable) ){
     /* For WITHOUT ROWID tables, ensure that all PRIMARY KEY columns are
-    ** either in the idxCols mask or in the extraCols mask */
+    ** either in the pkCols mask or in the extraCols mask. A column that
+    ** is already an index key but is not in pkCols is added a second
+    ** time, using the BINARY collating sequence. */
     for(i=0; i<pTable->nCol; i++){
       if( (pTable->aCol[i].colFlags & COLFLAG_PRIMKEY)==0 ) continue;
       if( i>=BMS-1 ){
         extraCols |= MASKBIT(BMS-1);
         break;
       }
-      if( idxCols & MASKBIT(i) ) continue;
+      if( pkCols & MASKBIT(i) ) continue;
       extraCols |= MASKBIT(i);
     }
   }
@@ -1115,7 +1138,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
   }
 
   /* Construct the Index object to describe this index */
-  assert( nKeyCol <= pTable->nCol + MAX(0, pTable->nCol - BMS + 1) );
+  assert( nKeyCol <= pTable->nCol + MIN(pTable->nCol, BMS) );
   /* ^-- This guarantees that the number of index columns will fit in the u16 */
   pIdx = sqlite3AllocateIndexObject(pParse->db, nKeyCol+HasRowid(pTable),
                                     0, &zNotUsed);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3AllocateIndexObject`
- 大写宏：`BMS`
- 大写宏：`COLFLAG_PRIMKEY`
- 大写宏：`KEY`
- 大写宏：`MASKBIT`
- 大写宏：`MAX`
- 大写宏：`PRIMARY`
- 大写宏：`ROWID`
- 大写宏：`WITHOUT`
- 外部类型：`Bitmap`
- 外部类型：`Bitmask`
- 外部类型：`Bloom`
- 外部类型：`Construct`
- 外部类型：`Expr`
- 外部类型：`Extra`
- 外部类型：`For`
- 外部类型：`Index`
- 外部类型：`Loop`
- 外部类型：`Make`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`WhereLoop`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-60d1092f14` → 本草稿移入 `cases/defect/auto-sqlite-60d1092f14/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
