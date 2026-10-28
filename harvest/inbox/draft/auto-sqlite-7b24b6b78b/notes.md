# auto-sqlite-7b24b6b78b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e5796f671ab939a8fcd0b3667931665ca40b030c](https://github.com/sqlite/sqlite/commit/e5796f671ab939a8fcd0b3667931665ca40b030c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e5796f671ab939a8fcd0b3667931665ca40b030c (https://github.com/sqlite/sqlite/commit/e5796f671ab939a8fcd0b3667931665ca40b030c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 531；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e5796f671ab939a8fcd0b3667931665ca40b030c Improve generate_series so that it handles constraints that compare :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -411,6 +411,13 @@ static sqlite3_int64 seriesRealToI64(double r){
   return (sqlite3_int64)r;
 }
 
+/* Return true if the type of the value is TEXT or BLOB.
+*/
+static int seriesIsTextOrBlob(sqlite3_value *pVal){
+  int eType = sqlite3_value_type(pVal);
+  return eType==SQLITE_TEXT || eType==SQLITE_BLOB;
+}
+
 /*
 ** This method is called to "rewind" the series_cursor object back
 ** to the first row of output.  This method is always called at least
@@ -525,10 +532,19 @@ static int seriesFilter(
 
   /* Narrow the range of iMin and iMax (the minimum and maximum outputs)
   ** based on equality and inequality constraints on the "value" column.
+  **
+  ** The "value" column has no affinity, so a TEXT or BLOB operand X is
+  ** not converted into a number.  Every INTEGER is less than any TEXT
+  ** or BLOB, so "value=X", "value>X" and "value>=X" are never true and
+  ** "value<X" and "value<=X" are always true.  The type must be checked
+  ** before sqlite3_value_numeric_type(), which would convert a TEXT
+  ** value such as '3' into a number.
   */
   if( idxNum & 0x3380 ){
     if( idxNum & 0x0080 ){    /* value=X */
-      if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
+      if( seriesIsTextOrBlob(argv[iArg]) ){
+        goto series_no_rows;
+      }else if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
         double r = sqlite3_value_double(argv[iArg++]);
         if( r==seriesCeil(r)
          && r>=(double)SMALLEST_INT64
@@ -543,7 +559,9 @@ static int seriesFilter(
       }
     }else{
       if( idxNum & 0x0300 ){  /* value>X (0x200) or value>=X (0x100) */
-        if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
+        if( seriesIsTextOrBlob(argv[iArg]) ){
+          goto series_no_rows;
+        }else if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
           double r = sqlite3_value_double(argv[iArg++]);
           if( r<=(double)SMALLEST_INT64 ){
             iMin = SMALLEST_INT64;
@@ -568,7 +586,9 @@ static int seriesFilter(
         }
       }
       if( idxNum & 0x3000 ){   /* value<X (0x2000) or value<=X (0x1000) */
-        if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
+        if( seriesIsTextOrBlob(argv[iArg]) ){
+          iArg++;   /* No upper bound */
+        }else if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
           double r = sqlite3_value_double(argv[iArg++]);
           if( r>=(double)LARGEST_INT64 ){
             iMax = LARGEST_INT64;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ceil`
- 外部函数：`sqlite3_value_double`
- 外部函数：`sqlite3_value_numeric_type`
- 大写宏：`SQLITE_FLOAT`
- 外部类型：`Narrow`
- 外部类型：`This`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7b24b6b78b` → 本草稿移入 `cases/defect/auto-sqlite-7b24b6b78b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
