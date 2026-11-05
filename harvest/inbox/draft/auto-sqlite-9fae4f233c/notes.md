# auto-sqlite-9fae4f233c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#4acc047df2e209ce749f2c5a4e19f6c471f63dbe](https://github.com/sqlite/sqlite/commit/4acc047df2e209ce749f2c5a4e19f6c471f63dbe) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 118 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4acc047df2e209ce749f2c5a4e19f6c471f63dbe (https://github.com/sqlite/sqlite/commit/4acc047df2e209ce749f2c5a4e19f6c471f63dbe)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 300；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4acc047df2e209ce749f2c5a4e19f6c471f63dbe Do not allow ALTER TABLE to add columns with CHECK constraints that use RAISE() expressions. Report [bugs:/info/2026-11- :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -297,7 +297,7 @@ static void sqlite3ErrorIfNotEmpty(
   const char *zErr      /* Error message text */
 ){
   sqlite3NestedParse(pParse,
-     "SELECT raise(ABORT,%Q) FROM \"%w\".\"%w\"",
+     "SELECT sqlite_fail(%Q, 1) FROM \"%w\".\"%w\"",
      zErr, zDb, zTab
   );
 }
@@ -516,10 +516,10 @@ void sqlite3AlterFinishAddColumn(Parse *pParse, Token *pColDef){
       pParse->colNamesSet = 1;
       sqlite3NestedParse(pParse,
         "SELECT CASE WHEN quick_check GLOB 'CHECK*'"
-        " THEN raise(ABORT,'CHECK constraint failed')"
+        " THEN sqlite_fail('CHECK constraint failed', 1)"
         " WHEN quick_check GLOB 'non-* value in*'"
-        " THEN raise(ABORT,'type mismatch on DEFAULT')"
-        " ELSE raise(ABORT,'NOT NULL constraint failed')"
+        " THEN sqlite_fail('type mismatch on DEFAULT', 1)"
+        " ELSE sqlite_fail('NOT NULL constraint failed', 1)"
         " END"
         "  FROM pragma_quick_check(%Q,%Q)"
         " WHERE quick_check GLOB 'CHECK*'"
@@ -3180,7 +3180,7 @@ void sqlite3AlterFunctions(void){
     INTERNAL_FUNCTION(sqlite_drop_constraint,2, dropConstraintFunc),
     INTERNAL_FUNCTION(sqlite_fail,           2, failConstraintFunc),
     INTERNAL_FUNCTION(sqlite_add_constraint, 3, addConstraintFunc),
-    INTERNAL_FUNCTION(sqlite_find_constraint,2, findConstraintFunc),
+    INTERNAL_FUNCTION(sqlite_find_constraint,2, findConstraintFunc)
   };
   sqlite3InsertBuiltinFuncs(aAlterTableFuncs, ArraySize(aAlterTableFuncs));
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`errorMPrintf`
- 外部函数：`memcpy`
- 外部函数：`pragma_quick_check`
- 外部函数：`raise`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3Dequote`
- 外部函数：`sqlite3GetToken`
- 外部函数：`sqlite3MallocZero`
- 外部函数：`sqlite3VMPrintf`
- 外部函数：`sqlite3_context_db_handle`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_result_error`
- 外部函数：`sqlite3_result_error_code`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_value_int`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`ABORT`
- 大写宏：`CASE`
- 大写宏：`CHECK`
- 大写宏：`COLLATE`
- 大写宏：`COMMA`
- 大写宏：`CONSTRAINT`
- 大写宏：`CREATE`
- 大写宏：`DEFAULT`
- 大写宏：`DEFERRABLE`
- 大写宏：`ELSE`
- 大写宏：`END`
- 大写宏：`FOREIGN`
- 大写宏：`FROM`
- 大写宏：`GENERATED`
- 大写宏：`GLOB`
- 大写宏：`ILLEGAL`
- 大写宏：`KEY`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`OUT`
- 大写宏：`PRIMARY`
- 大写宏：`REFERENCES`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_FOREIGN_KEY`
- 大写宏：`TABLE`
- 大写宏：`THEN`
- 大写宏：`TK_AS`
- 大写宏：`TK_CHECK`
- 大写宏：`TK_COLLATE`
- 大写宏：`TK_COMMA`
- 大写宏：`TK_COMMENT`
- 大写宏：`TK_CONSTRAINT`
- 大写宏：`TK_DEFAULT`
- 大写宏：`TK_DEFERRABLE`
- 大写宏：`TK_FOREIGN`
- 大写宏：`TK_GENERATED`
- 大写宏：`TK_ILLEGAL`
- 大写宏：`TK_LP`
- 大写宏：`TK_NOT`
- 大写宏：`TK_PRIMARY`
- 大写宏：`TK_REFERENCES`
- 大写宏：`TK_RP`
- 大写宏：`TK_SET`
- 大写宏：`TK_SPACE`
- 大写宏：`TK_UNIQUE`
- 大写宏：`UNIQUE`
- 大写宏：`UNUSED_PARAMETER`
- 大写宏：`WHEN`
- 大写宏：`WHERE`
- 外部类型：`Also`
- 外部类型：`De`
- 外部类型：`Do`
- 外部类型：`Error`
- 外部类型：`Exceptions`
- 外部类型：`For`
- 外部类型：`Function`
- 外部类型：`Gobble`
- 外部类型：`If`
- 外部类型：`Jump`
- 外部类型：`Length`
- 外部类型：`Not`
- 外部类型：`NotUsed`
- 外部类型：`Now`
- 外部类型：`Possibly`
- 外部类型：`Set`
- 外部类型：`Then`
- 外部类型：`Token`
- 外部类型：`Zero`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-9fae4f233c` → 本草稿移入 `cases/defect/auto-sqlite-9fae4f233c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
