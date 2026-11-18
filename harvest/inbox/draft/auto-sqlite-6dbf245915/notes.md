# auto-sqlite-6dbf245915

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#90ba7148c6d943ec5233002927c18e57cc8a5589](https://github.com/sqlite/sqlite/commit/90ba7148c6d943ec5233002927c18e57cc8a5589) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #90ba7148c6d943ec5233002927c18e57cc8a5589 (https://github.com/sqlite/sqlite/commit/90ba7148c6d943ec5233002927c18e57cc8a5589)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 1138；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 90ba7148c6d943ec5233002927c18e57cc8a5589 Merge all recent trunk changes into this branch. :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -30,7 +30,12 @@
 **
 ** returns the allowed vocabulary for the match.  If the "vocabulary_language"
 ** or L columnname is left unspecified or is an empty string, then no
-** filtering of the vocabulary by language is performed. 
+** filtering of the vocabulary by language is performed.
+**
+** Table V must be either in the "temp" schema or in the same database as
+** the amatch virtual table itself. If a table or view named V exists in
+** "temp", it is used. The edit-cost-table must be in the same database as
+** the amatch virtual table.
 **
 ** For efficiency, it is essential that the vocabulary table be indexed:
 **
@@ -1111,6 +1116,29 @@ static void amatchAddWord(
 }
 
 
+/*
+** Return the name of the schema to read the vocabulary table from. This
+** is "temp" if there is a table or view with the right name in the temp
+** schema, or the schema containing the amatch virtual table otherwise.
+** Attached databases other than the one containing the amatch table are
+** never searched.
+*/
+static const char *amatchVocabSchema(amatch_vtab *p){
+  const char *zRet = p->zDb;
+  sqlite3_stmt *pStmt = 0;
+  int rc = sqlite3_prepare_v2(p->db,
+      "SELECT 1 FROM temp.sqlite_schema"
+      " WHERE type IN ('table','view') AND name=?1 COLLATE nocase",
+      -1, &pStmt, 0
+  );
+  if( rc==SQLITE_OK ){
+    sqlite3_bind_text(pStmt, 1, p->zVocabTab, -1, SQLITE_STATIC);
+    if( sqlite3_step(pStmt)==SQLITE_ROW ) zRet = "temp";
+  }
+  sqlite3_finalize(pStmt);
+  return zRet;
+}
+
 /*
 ** Advance a cursor to its next row of output
 */
@@ -1133,20 +1161,21 @@ static int amatchNext(sqlite3_vtab_cursor *cur){
 
   if( p->pVCheck==0 ){
     char *zSql;
+    const char *zSchema = amatchVocabSchema(p);
     if( p->zVocabLang && p->zVocabLang[0] ){
       zSql = sqlite3_mprintf(
-          "SELECT \"%w\" FROM \"%w\"",
+          "SELECT \"%w\" FROM \"%w\".\"%w\""
           " WHERE \"%w\">=?1 AND \"%w\"=?2"
           " ORDER BY 1",
-          p->zVocabWord, p->zVocabTab,
+          p->zVocabWord, zSchema, p->zVocabTab,
           p->zVocabWord, p->zVocabLang
       );
     }else{
       zSql = sqlite3_mprintf(
-          "SELECT \"%w\" FROM \"%w\""
+          "SELECT \"%w\" FROM \"%w\".\"%w\""
           " WHERE \"%w\">=?1"
           " ORDER BY 1",
-          p->zVocabWord, p->zVocabTab,
+          p->zVocabWord, zSchema, p->zVocabTab,
           p->zVocabWord
       );
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_mprintf`
- 大写宏：`AND`
- 大写宏：`FROM`
- 大写宏：`ORDER`
- 大写宏：`SELECT`
- 大写宏：`WHERE`
- 外部类型：`Advance`
- 外部类型：`For`
- 外部类型：`If`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6dbf245915` → 本草稿移入 `cases/defect/auto-sqlite-6dbf245915/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
