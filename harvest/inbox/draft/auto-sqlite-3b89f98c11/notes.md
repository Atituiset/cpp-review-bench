# auto-sqlite-3b89f98c11

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cb5651e46da369e8245d69dd6959cf6d5050068a](https://github.com/sqlite/sqlite/commit/cb5651e46da369e8245d69dd6959cf6d5050068a) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 70 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cb5651e46da369e8245d69dd6959cf6d5050068a (https://github.com/sqlite/sqlite/commit/cb5651e46da369e8245d69dd6959cf6d5050068a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 5351；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cb5651e46da369e8245d69dd6959cf6d5050068a Fix integrity-check fails that would occur if the database contained a contentless fts4 table. Report [bugs:/info/2026-1 :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5329,6 +5329,7 @@ int sqlite3Fts3IntegrityCheck(Fts3Table *p, int *pbOk){
   u64 cksum1 = 0;                 /* Checksum based on FTS index contents */
   u64 cksum2 = 0;                 /* Checksum based on %_content contents */
   sqlite3_stmt *pAllLangid = 0;   /* Statement to return all language-ids */
+  int bContentless = (p->zContentTbl && p->zContentTbl[0]=='\0');
 
   /* This block calculates the checksum according to the FTS index. */
   rc = fts3SqlStmt(p, SQL_SELECT_ALL_LANGID, &pAllLangid, 0);
@@ -5348,7 +5349,7 @@ int sqlite3Fts3IntegrityCheck(Fts3Table *p, int *pbOk){
   }
 
   /* This block calculates the checksum according to the %_content table */
-  if( rc==SQLITE_OK ){
+  if( rc==SQLITE_OK && !bContentless ){
     sqlite3_tokenizer_module const *pModule = p->pTokenizer->pModule;
     sqlite3_stmt *pStmt = 0;
     char *zSql;
@@ -5406,7 +5407,7 @@ int sqlite3Fts3IntegrityCheck(Fts3Table *p, int *pbOk){
     rc = SQLITE_OK;
     *pbOk = 0;
   }else{
-    *pbOk = (rc==SQLITE_OK && cksum1==cksum2);
+    *pbOk = (rc==SQLITE_OK && (bContentless || cksum1==cksum2));
   }
   return rc;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`coalesce`
- 外部函数：`count`
- 外部函数：`max`
- 外部函数：`root`
- 外部函数：`sqlite3Fts3Incrmerge`
- 外部函数：`sqlite3_bind_parameter_count`
- 外部函数：`sqlite3_bind_value`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v3`
- 外部函数：`start_block`
- 外部函数：`total`
- 大写宏：`AND`
- 大写宏：`ASC`
- 大写宏：`BETWEEN`
- 大写宏：`DELETE`
- 大写宏：`DESC`
- 大写宏：`EXISTS`
- 大写宏：`FAIL`
- 大写宏：`FROM`
- 大写宏：`FTS`
- 大写宏：`GROUP`
- 大写宏：`HAVING`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LIMIT`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`REPLACE`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQL`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_PREPARE_FROM_DDL`
- 大写宏：`SQLITE_PREPARE_NO_VTAB`
- 大写宏：`SQLITE_PREPARE_PERSISTENT`
- 大写宏：`SQL_CHOMP_SEGDIR`
- 大写宏：`SQL_DELETE_SEGDIR_ENTRY`
- 大写宏：`SQL_SEGMENT_IS_APPENDABLE`
- 大写宏：`SQL_SELECT_INDEXES`
- 大写宏：`SQL_SELECT_MXLEVEL`
- 大写宏：`SQL_SELECT_SEGDIR`
- 大写宏：`SQL_SHIFT_SEGDIR_ENTRY`
- 大写宏：`SQL_XXX`
- 大写宏：`UNION`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Checksum`
- 外部类型：`Delete`
- 外部类型：`Estimate`
- 外部类型：`Fts3Table`
- 外部类型：`It`
- 外部类型：`Modify`
- 外部类型：`One`
- 外部类型：`Or`
- 外部类型：`Prepare`
- 外部类型：`Prepared`
- 外部类型：`Read`
- 外部类型：`Return`
- 外部类型：`See`
- 外部类型：`Statement`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Update`
- 外部类型：`Values`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-3b89f98c11` → 本草稿移入 `cases/defect/auto-sqlite-3b89f98c11/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
