# auto-sqlite-8d5da60433

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#6fd45c5ce98159eeb49421092672b3d31447a173](https://github.com/sqlite/sqlite/commit/6fd45c5ce98159eeb49421092672b3d31447a173) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 54 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6fd45c5ce98159eeb49421092672b3d31447a173 (https://github.com/sqlite/sqlite/commit/6fd45c5ce98159eeb49421092672b3d31447a173)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1615；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6fd45c5ce98159eeb49421092672b3d31447a173 Avoid a signed integer overflow in fts5 when processing a corrupt database. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1590,6 +1590,17 @@ static void fts5StructurePromote(
   }
 }
 
+/*
+** Return the page number iPgno+nDelta. If this is larger than FTS5_MAX_PGNO,
+** which can only happen if the doclist-index record is corrupt, return
+** FTS5_MAX_PGNO+1 instead. No leaf page with that number may exist, so
+** this causes the corruption to be detected when the caller attempts to
+** use the page number.
+*/
+static int fts5AddPgno(int iPgno, int nDelta){
+  i64 iRet = (i64)iPgno + nDelta;
+  return (int)(iRet>FTS5_MAX_PGNO ? FTS5_MAX_PGNO+1 : iRet);
+}
 
 /*
 ** Advance the iterator passed as the only argument. If the end of the 
@@ -1612,7 +1623,7 @@ static int fts5DlidxLvlNext(Fts5DlidxLvl *pLvl){
 
     if( iOff<pData->nn ){
       u64 iVal;
-      pLvl->iLeafPgno += (iOff - pLvl->iOff) + 1;
+      pLvl->iLeafPgno = fts5AddPgno(pLvl->iLeafPgno, iOff-pLvl->iOff+1);
       iOff += fts5GetVarint(&pData->p[iOff], &iVal);
       pLvl->iRowid += iVal;
       pLvl->iOff = iOff;
@@ -1722,7 +1733,7 @@ static int fts5DlidxLvlPrev(Fts5DlidxLvl *pLvl){
       ii += sqlite3Fts5GetVarint(&a[ii], &delta);
 
       if( ii>=iOff ) break;
-      pLvl->iLeafPgno += nZero+1;
+      pLvl->iLeafPgno = fts5AddPgno(pLvl->iLeafPgno, nZero+1);
       pLvl->iRowid += delta;
       pLvl->iOff = ii;
     }
@@ -8615,7 +8626,9 @@ static void fts5IndexIntegrityCheckSegment(
       ){
 
         /* Check any rowid-less pages that occur before the current leaf. */
-        for(iPg=iPrevLeaf+1; iPg<fts5DlidxIterPgno(pDlidx); iPg++){
+        for(iPg=iPrevLeaf+1; p->rc==SQLITE_OK && iPg<fts5DlidxIterPgno(pDlidx);
+            iPg++
+        ){
           iKey = FTS5_SEGMENT_ROWID(iSegid, iPg);
           pLeaf = fts5LeafRead(p, iKey);
           if( pLeaf ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`flag`
- 外部函数：`fts5DataXXX`
- 外部函数：`fts5GetU16`
- 外部函数：`fts5GetVarint`
- 外部函数：`fts5_dri`
- 外部函数：`memset`
- 外部函数：`sqlite3Fts5ConfigErrmsg`
- 外部函数：`sqlite3Fts5GetVarint`
- 外部函数：`sqlite3_blob_bytes`
- 外部函数：`sqlite3_blob_close`
- 外部函数：`sqlite3_blob_open`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_blob_reopen`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`structure`
- 大写宏：`AND`
- 大写宏：`DELETE`
- 大写宏：`EIGHT_BYTE_ALIGNMENT`
- 大写宏：`FROM`
- 大写宏：`FTS5_CORRUPT`
- 大写宏：`INSERT`
- 大写宏：`NULL`
- 大写宏：`SQLITE_ABORT`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Advance`
- 外部类型：`All`
- 外部类型：`Check`
- 外部类型：`Current`
- 外部类型：`Doclist`
- 外部类型：`Error`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Data`
- 外部类型：`Fts5DlidxIter`
- 外部类型：`Fts5Hash`
- 外部类型：`Fts5Index`
- 外部类型：`Fts5Structure`
- 外部类型：`Hash`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Leaf`
- 外部类型：`Max`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`Read`
- 外部类型：`Rowid`
- 外部类型：`State`
- 外部类型：`This`
- 外部类型：`Total`
- 外部类型：`Variables`
- 外部类型：`Virtual`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-8d5da60433` → 本草稿移入 `cases/defect/auto-sqlite-8d5da60433/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
