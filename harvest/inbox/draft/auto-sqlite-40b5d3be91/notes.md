# auto-sqlite-40b5d3be91

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#62c805e36c43e015dce80f3a5d15ac528abf2584](https://github.com/sqlite/sqlite/commit/62c805e36c43e015dce80f3a5d15ac528abf2584) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #62c805e36c43e015dce80f3a5d15ac528abf2584 (https://github.com/sqlite/sqlite/commit/62c805e36c43e015dce80f3a5d15ac528abf2584)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 342；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 62c805e36c43e015dce80f3a5d15ac528abf2584 Cygwin portability patches from Jan Nijtmans. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -339,7 +339,7 @@ static char *fuzz_invariant_sql(sqlite3_stmt *pStmt, int iCnt){
   zIn = sqlite3_sql(pStmt);
   if( zIn==0 ) return 0;
   nIn = strlen(zIn);
-  while( nIn>0 && (isspace(zIn[nIn-1]) || zIn[nIn-1]==';') ) nIn--;
+  while( nIn>0 && (isspace((unsigned char)zIn[nIn-1]) || zIn[nIn-1]==';') ) nIn--;
   if( strchr(zIn, '?') ) return 0;
   pTest = sqlite3_str_new(0);
   sqlite3_str_appendf(pTest, "SELECT %s* FROM (",  
@@ -357,8 +357,8 @@ static char *fuzz_invariant_sql(sqlite3_stmt *pStmt, int iCnt){
     const char *zColName = sqlite3_column_name(pBase,i);
     const char *zSuffix = zColName ? strrchr(zColName, ':') : 0;
     if( zSuffix 
-     && isdigit(zSuffix[1])
-     && (zSuffix[1]>'3' || isdigit(zSuffix[2]))
+     && isdigit((unsigned char)zSuffix[1])
+     && (zSuffix[1]>'3' || isdigit((unsigned char)zSuffix[2]))
     ){
       /* This is a randomized column name and so cannot be used in the
       ** WHERE clause. */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`isdigit`
- 外部函数：`isspace`
- 外部函数：`sqlite3_column_name`
- 外部函数：`sqlite3_sql`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`sqlite3_str_new`
- 外部函数：`strchr`
- 外部函数：`strlen`
- 外部函数：`strrchr`
- 大写宏：`FROM`
- 大写宏：`SELECT`
- 大写宏：`WHERE`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-40b5d3be91` → 本草稿移入 `cases/defect/auto-sqlite-40b5d3be91/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
