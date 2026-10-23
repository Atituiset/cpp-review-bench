# auto-sqlite-6784e54158

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#20822819adf299e3a7609a26206bf2b43ac402ff](https://github.com/sqlite/sqlite/commit/20822819adf299e3a7609a26206bf2b43ac402ff) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 61（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #20822819adf299e3a7609a26206bf2b43ac402ff (https://github.com/sqlite/sqlite/commit/20822819adf299e3a7609a26206bf2b43ac402ff)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1936；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 20822819adf299e3a7609a26206bf2b43ac402ff Fix two minor issues with the geopoly extension. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1908,6 +1908,20 @@ static int deserializeGeometry(sqlite3_value *pValue, RtreeConstraint *pCons){
 
 int sqlite3IntFloatCompare(i64,double);
 
+/*
+** Argument pVal is the right-hand side of a "rowid = ?" constraint. If
+** there is an integer that compares equal to pVal, set *piRowid to it and
+** return 1. Otherwise - pVal is NULL, a blob, a non-numeric text value or
+** a real value that is not an integer - return 0.
+*/
+static int rtreeRowidEqValue(sqlite3_value *pVal, i64 *piRowid){
+  int eType = sqlite3_value_numeric_type(pVal);
+  *piRowid = sqlite3_value_int64(pVal);
+  return eType==SQLITE_INTEGER
+      || (eType==SQLITE_FLOAT
+          && 0==sqlite3IntFloatCompare(*piRowid, sqlite3_value_double(pVal)));
+}
+
 /* 
 ** Rtree virtual table module xFilter method.
 */
@@ -1933,13 +1947,9 @@ static int rtreeFilter(
     /* Special case - lookup by rowid. */
     RtreeNode *pLeaf;        /* Leaf on which the required cell resides */
     RtreeSearchPoint *p;     /* Search point for the leaf */
-    i64 iRowid = sqlite3_value_int64(argv[0]);
+    i64 iRowid;
     i64 iNode = 0;
-    int eType = sqlite3_value_numeric_type(argv[0]);
-    if( eType==SQLITE_INTEGER
-     || (eType==SQLITE_FLOAT 
-         && 0==sqlite3IntFloatCompare(iRowid,sqlite3_value_double(argv[0])))
-    ){
+    if( rtreeRowidEqValue(argv[0], &iRowid) ){
       rc = findLeafNode(pRtree, iRowid, &pLeaf, &iNode);
     }else{
       rc = SQLITE_OK;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3IntFloatCompare`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_blob_bytes`
- 外部函数：`sqlite3_blob_close`
- 外部函数：`sqlite3_blob_open`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_blob_reopen`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_int64`
- 外部函数：`sqlite3_value_numeric_type`
- 大写宏：`NULL`
- 大写宏：`OUT`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 外部类型：`Acquired`
- 外部类型：`Check`
- 外部类型：`Either`
- 外部类型：`If`
- 外部类型：`Leaf`
- 外部类型：`Node`
- 外部类型：`RTree`
- 外部类型：`Rtree`
- 外部类型：`RtreeNode`
- 外部类型：`RtreeSearchPoint`
- 外部类型：`Search`
- 外部类型：`Special`
- 外部类型：`The`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6784e54158` → 本草稿移入 `cases/defect/auto-sqlite-6784e54158/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
