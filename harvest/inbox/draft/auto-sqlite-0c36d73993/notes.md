# auto-sqlite-0c36d73993

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d9af1e23f1c480b3a38812f1009d222ed1f24609](https://github.com/sqlite/sqlite/commit/d9af1e23f1c480b3a38812f1009d222ed1f24609) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d9af1e23f1c480b3a38812f1009d222ed1f24609 (https://github.com/sqlite/sqlite/commit/d9af1e23f1c480b3a38812f1009d222ed1f24609)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 26（原始 PR diff 行 1136；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d9af1e23f1c480b3a38812f1009d222ed1f24609 Fix a problem with automatic indexes on WITHOUT ROWID tables where at least one field of the PRIMARY KEY is its 63rd or  :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1013,6 +1013,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
   Bitmask idxCols;            /* Bitmap of columns used for indexing */
   Bitmask pkCols;             /* PK cols whose key collation keeps them unique */
   Bitmask extraCols;          /* Bitmap of additional columns */
+  int bHighCols;              /* True to add all columns >= BMS-1 */
   u8 sentWarning = 0;         /* True if a warning has been issued */
   u8 useBloomFilter = 1;      /* True to also add a Bloom filter */
   Expr *pPartial = 0;         /* Partial Index Expression */
@@ -1112,6 +1113,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
   }else{
     extraCols = pSrc->colUsed & (~idxCols | MASKBIT(BMS-1));
   }
+  bHighCols = (pSrc->colUsed & MASKBIT(BMS-1))!=0;
   if( !HasRowid(pTable) ){
     /* For WITHOUT ROWID tables, ensure that all PRIMARY KEY columns are
     ** either in the pkCols mask or in the extraCols mask. A column that
@@ -1120,7 +1122,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
     for(i=0; i<pTable->nCol; i++){
       if( (pTable->aCol[i].colFlags & COLFLAG_PRIMKEY)==0 ) continue;
       if( i>=BMS-1 ){
-        extraCols |= MASKBIT(BMS-1);
+        bHighCols = 1;
         break;
       }
       if( pkCols & MASKBIT(i) ) continue;
@@ -1133,7 +1135,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
   for(i=0; i<mxBitCol; i++){
     if( extraCols & MASKBIT(i) ) nKeyCol++;
   }
-  if( pSrc->colUsed & MASKBIT(BMS-1) ){
+  if( bHighCols ){
     nKeyCol += pTable->nCol - BMS + 1;
   }
 
@@ -1184,7 +1186,7 @@ static SQLITE_NOINLINE void constructAutomaticIndex(
       n++;
     }
   }
-  if( pSrc->colUsed & MASKBIT(BMS-1) ){
+  if( bHighCols ){
     for(i=BMS-1; i<pTable->nCol; i++){
       pIdx->aiColumn[n] = i;
       pIdx->azColl[n] = sqlite3StrBINARY;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`BMS`
- 大写宏：`COLFLAG_PRIMKEY`
- 大写宏：`KEY`
- 大写宏：`MASKBIT`
- 大写宏：`PRIMARY`
- 大写宏：`ROWID`
- 大写宏：`WITHOUT`
- 外部类型：`Bitmap`
- 外部类型：`Bitmask`
- 外部类型：`Bloom`
- 外部类型：`Expr`
- 外部类型：`Expression`
- 外部类型：`For`
- 外部类型：`Index`
- 外部类型：`Partial`
- 外部类型：`True`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-0c36d73993` → 本草稿移入 `cases/defect/auto-sqlite-0c36d73993/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
