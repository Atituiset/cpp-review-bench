# auto-sqlite-807a751924

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cc758d2fba50f108d479c642f68a5ebb70252df3](https://github.com/sqlite/sqlite/commit/cc758d2fba50f108d479c642f68a5ebb70252df3) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 130（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cc758d2fba50f108d479c642f68a5ebb70252df3 (https://github.com/sqlite/sqlite/commit/cc758d2fba50f108d479c642f68a5ebb70252df3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 11889；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cc758d2fba50f108d479c642f68a5ebb70252df3 Fix a vec1 problem with issuing a 'rebuild' command within a transaction that has already written to the table. Report [ :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -11415,6 +11415,17 @@ static int vec1FixVectorSize(Vec1Tab *pTab, int nVec){
 }
 
 
+static int vec1FinishWriter(Vec1Tab *pTab, int bDiscard){
+  int rc = SQLITE_OK;
+  if( bDiscard ){
+    vec1WriterFinish(pTab->pWriter, SQLITE_ERROR);
+  }else{
+    rc = vec1WriterFinish(pTab->pWriter, SQLITE_OK);
+  }
+  pTab->pWriter = 0;
+  return rc;
+}
+
 /*
 ** Handle a "special" INSERT command on a vec1 table. A special INSERT takes
 ** the form:
@@ -11437,6 +11448,9 @@ static int vec1SpecialInsert(
   int iCmd = 0;
 
   rc = vec1InterpretCmdName(pTab, zCmd, &iCmd);
+  if( rc==SQLITE_OK ){
+    rc = vec1FinishWriter(pTab, 0);
+  }
 
   if( rc==SQLITE_OK ){
     switch( iCmd ){
@@ -11886,17 +11900,6 @@ static int vec1UpdateMethod(
   return rc;
 }
 
-static int vec1FinishWriter(Vec1Tab *pTab, int bDiscard){
-  int rc = SQLITE_OK;
-  if( bDiscard ){
-    vec1WriterFinish(pTab->pWriter, SQLITE_ERROR);
-  }else{
-    rc = vec1WriterFinish(pTab->pWriter, SQLITE_OK);
-  }
-  pTab->pWriter = 0;
-  return rc;
-}
-
 static int vec1SyncMethod(sqlite3_vtab *pVtab){
   int rc = SQLITE_OK;
   Vec1Tab *pTab = (Vec1Tab*)pVtab;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`length`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`size_of_array`
- 外部函数：`sqlite3_bind_blob`
- 外部函数：`sqlite3_bind_int`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_bind_null`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_last_insert_rowid`
- 外部函数：`sqlite3_malloc`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sum`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vec1CorruptError`
- 大写宏：`AND`
- 大写宏：`BETWEEN`
- 大写宏：`CONFLICT`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LIMIT`
- 大写宏：`ORDER`
- 大写宏：`REPLACE`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Buffer`
- 外部类型：`Handle`
- 外部类型：`Id`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Same`
- 外部类型：`Set`
- 外部类型：`So`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Used`
- 外部类型：`Vec1`
- 外部类型：`Vec1Buffer`
- 外部类型：`Vec1ListBuilder`
- 外部类型：`Vec1MetaBuilder`
- 外部类型：`Vec1MetaValue`
- 外部类型：`Vec1Tab`
- 外部类型：`Vec1Writer`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-807a751924` → 本草稿移入 `cases/defect/auto-sqlite-807a751924/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
