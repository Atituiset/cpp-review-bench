# auto-sqlite-c03fa0112b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#80a6cb85686e6fabaab23b318bce66087e782b86](https://github.com/sqlite/sqlite/commit/80a6cb85686e6fabaab23b318bce66087e782b86) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #80a6cb85686e6fabaab23b318bce66087e782b86 (https://github.com/sqlite/sqlite/commit/80a6cb85686e6fabaab23b318bce66087e782b86)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 3783；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 80a6cb85686e6fabaab23b318bce66087e782b86 Fix some integer overflow problems in fts3/4. Report [bugs:/info/2026-10-29T14:48:44Z | 2026-10-29T14:48:44Z]. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3780,9 +3780,9 @@ struct NodeReader {
 ** unmodified. Otherwise, if the allocation succeeds, update pBlob->nAlloc
 ** to reflect the new size of the pBlob->a[] buffer.
 */
-static void blobGrowBuffer(Blob *pBlob, int nMin, int *pRc){
+static void blobGrowBuffer(Blob *pBlob, i64 nMin, int *pRc){
   if( *pRc==SQLITE_OK && nMin>pBlob->nAlloc ){
-    int nAlloc = nMin;
+    i64 nAlloc = nMin;
     char *a = (char *)sqlite3_realloc64(pBlob->a, nAlloc);
     if( a ){
       pBlob->nAlloc = nAlloc;
@@ -4050,7 +4050,7 @@ static int fts3IncrmergeAppend(
   const char *aDoclist = pCsr->aDoclist;
   int nDoclist = pCsr->nDoclist;
   int rc = SQLITE_OK;           /* Return code */
-  int nSpace;                   /* Total space in bytes required on leaf */
+  i64 nSpace;                   /* Total space in bytes required on leaf */
   int nPrefix;                  /* Size of prefix shared with previous term */
   int nSuffix;                  /* Size of suffix (nTerm - nPrefix) */
   NodeWriter *pLeaf;            /* Object used to write leaf nodes */
@@ -4060,9 +4060,9 @@ static int fts3IncrmergeAppend(
   nSuffix = nTerm - nPrefix;
   if(nSuffix<=0 ) return FTS_CORRUPT_VTAB;
 
-  nSpace  = sqlite3Fts3VarintLen(nPrefix);
-  nSpace += sqlite3Fts3VarintLen(nSuffix) + nSuffix;
-  nSpace += sqlite3Fts3VarintLen(nDoclist) + nDoclist;
+  nSpace  = (i64)sqlite3Fts3VarintLen(nPrefix);
+  nSpace += (i64)sqlite3Fts3VarintLen(nSuffix) + nSuffix;
+  nSpace += (i64)sqlite3Fts3VarintLen(nDoclist) + nDoclist;
 
   /* If the current block is not empty, and if adding this term/doclist
   ** to the current block would make it larger than Fts3Table.nNodeSize bytes,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3Fts3VarintLen`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`suffix`
- 大写宏：`FTS_CORRUPT_VTAB`
- 大写宏：`SQLITE_OK`
- 外部类型：`Blob`
- 外部类型：`Fts3Table`
- 外部类型：`If`
- 外部类型：`NodeWriter`
- 外部类型：`Object`
- 外部类型：`Otherwise`
- 外部类型：`Return`
- 外部类型：`Size`
- 外部类型：`Total`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-c03fa0112b` → 本草稿移入 `cases/defect/auto-sqlite-c03fa0112b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
