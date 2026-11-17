# auto-sqlite-59ccbf154c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#530016b058d1e18e6ad222294395a759c953fa88](https://github.com/sqlite/sqlite/commit/530016b058d1e18e6ad222294395a759c953fa88) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #530016b058d1e18e6ad222294395a759c953fa88 (https://github.com/sqlite/sqlite/commit/530016b058d1e18e6ad222294395a759c953fa88)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 530016b058d1e18e6ad222294395a759c953fa88 New debugging interface, callable from gdb or similar: :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1287,6 +1287,920 @@ void sqlite3TreeViewTrigger(
 #endif /* SQLITE_OMIT_TRIGGER */
   
 
+/*****************************************************************************
+** Regenerate SQL text from a Select object.
+**
+** Intended for interactive use in a debugger:
+**
+**      (gdb) call sqlite3ShowSelectAsSql(p)
+**
+** This is not an API.  This code is subject to change or removal at any
+** time.  This code is not present in production builds.  The SQLite core
+** never uses this code.
+**
+** The output is on a single line and is intended to have the same
+** meaning as the original SQL, though not the same text.  The parser
+** (and later the resolver and optimizer) discard or rewrite information,
+** so the following are only approximate:
+**
+**    *  Parentheses are regenerated from operator precedence.
+**    *  Column references that have been resolved are shown as
+**       ALIAS.COLUMN, looked up through the FROM clauses of the enclosing
+**       SELECT statements.
+**    *  A "*" that has already been expanded is shown expanded.
+**    *  NATURAL joins that have been converted into USING, and the
+**       ON-to-WHERE movement of the query flattener, show the converted
+**       form.
+**    *  Window-function rewrites (SF_WinRewrite) and other transformations
+**       that synthesize new subqueries are shown as the transformed query.
+**    *  Anything not understood is shown as <...>, which is not SQL.
+*/
+typedef struct ShowSqlScope ShowSqlScope;
+struct ShowSqlScope {
+  const SrcList *pSrc;           /* A FROM clause that is in scope */
+  const ShowSqlScope *pUp;       /* FROM clauses of enclosing SELECTs */
+};
+typedef struct ShowSql ShowSql;
+struct ShowSql {
+  sqlite3_str *pOut;             /* Append SQL text here */
+  const ShowSqlScope *pScope;    /* Innermost FROM clause in scope */
+};
+
+/* Flags for showSqlExprList() */
+#define SHOWSQL_ALIAS   0x01     /* Show AS names (result set) */
+#define SHOWSQL_SORT    0x02     /* Show ASC/DESC/NULLS (ORDER BY) */
+
+static void showSqlExpr(ShowSql*, const Expr*, int);
+static void showSqlExprList(ShowSql*, const ExprList*, int);
+static void showSqlSelect(ShowSql*, const Select*);
+static void showSqlSrcList(ShowSql*, const SrcList*);
+
+/* Operator precedences.  Larger numbers bind more tightly. */
+#define SQLPREC_OR        1
+#define SQLPREC_AND       2
+#define SQLPREC_NOT       3
+#define SQLPREC_EQ        4   /* = <> IS BETWEEN IN LIKE ISNULL ... */
+#define SQLPREC_CMP       5   /* < <= > >= */
+#define SQLPREC_BIT       7   /* & | << >> */
+#define SQLPREC_ADD       8
+#define SQLPREC_MUL       9
+#define SQLPREC_CONCAT   10   /* || -> ->> */
+#define SQLPREC_COLLATE  11
+#define SQLPREC_UNARY    12   /* ~ + - */
+#define SQLPREC_ATOM     99
+
+/* Append the identifier z, with quotes if they are necessary */
+static void showSqlId(sqlite3_str *p, const char *z){
+  int i, n;
+  int bOk;
+  if( z==0 ){
+    sqlite3_str_appendall(p, "<null-id>");
+    return;
+  }
+  n = (int)strlen(z);
+  bOk = n>0 && (sqlite3Isalpha(z[0]) || z[0]=='_');
+  for(i=1; bOk && i<n; i++){
+    if( !sqlite3Isalnum(z[i]) && z[i]!='_' && z[i]!='$' ) bOk = 0;
+  }
+  if( bOk && sqlite3_keyword_check(z, n) ) bOk = 0;
+  if( bOk ){
+    sqlite3_str_appendall(p, z);
+  }else{
+    sqlite3_str_appendf(p, "\"%w\"", z);
+  }
+}
+
+/* Append a name in upper-case */
+static void showSqlUpper(sqlite3_str *p, const char *z){
+  for(; *z; z++){
+    sqlite3_str_appendchar(p, 1, (char)sqlite3Toupper(*z));
+  }
+}
+
+static void showSqlIdList(sqlite3_str *p, const IdList *pIds){
+  int i;
+  if( pIds==0 ) return;
+  for(i=0; i<pIds->nId; i++){
+    if( i ) sqlite3_str_append(p, ",", 1);
+    showSqlId(p, pIds->a[i].zName);
+  }
+}
+
+/* Find the FROM-clause entry with cursor number iCursor, if it is in scope */
+static const SrcItem *showSqlFindSrc(const ShowSqlScope *pScope, int iCursor){
+  for(; pScope; pScope=pScope->pUp){
+    const SrcList *pSrc = pScope->pSrc;
+    int 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`SQLITE_OMIT_TRIGGER`
- 外部类型：`These`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-59ccbf154c` → 本草稿移入 `cases/defect/auto-sqlite-59ccbf154c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
