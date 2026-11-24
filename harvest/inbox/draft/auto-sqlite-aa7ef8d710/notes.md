# auto-sqlite-aa7ef8d710

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d169f045ba96514b362d33f8c84489dd37cdaceb](https://github.com/sqlite/sqlite/commit/d169f045ba96514b362d33f8c84489dd37cdaceb) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d169f045ba96514b362d33f8c84489dd37cdaceb (https://github.com/sqlite/sqlite/commit/d169f045ba96514b362d33f8c84489dd37cdaceb)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 76；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d169f045ba96514b362d33f8c84489dd37cdaceb Merge recent trunk enhancements into the no-writer branch :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -14,7 +14,7 @@
 ** production use.  The interface might change.  This file might go out
 ** of support and disappear at any moment.  Do not depend on this extension.
 **
-** Apparent from comments in the code, this extension is undocumented.
+** Apart from comments in the code, this extension is undocumented.
 **
 ** This SQLite extension implements the SQL function tryeval(). The tryeval()
 ** function takes zero or more arguments, each of which is an SQL statement.
@@ -29,6 +29,10 @@
 ** Update:  Also the expire(V) and expire(V,N) functions.  See comments
 ** below for details.
 **
+** Update (2026-11-24):  If any argument to tryeval() is the magic string
+** "FAIL" then if the next argument returns an error, that error is
+** propagated up to the caller of tryeval() instead of being ignored.
+**
 ** The purpose of this function is to help evaluate recursive
 ** behavior, and expecially the bug report at
 ** sqlite.org/bugs/info/3577bc69c52237a4 (2026-10-25T19:26:00Z)
@@ -59,11 +63,16 @@ static void tryEvalFunc(
   int bDone = 0;          /* True if a row with non-NULL first column seen */
   int rc;                 /* Result from sqlite3 API calls */
   int i;                  /* Loop counter */
+  int bFail = 0;          /* True if "FAIL" seen */
 
   db = sqlite3_context_db_handle(context);
   for(i=0; !bDone && i<argc; i++){
     zSql = (const char*)sqlite3_value_text(argv[i]);
     if( zSql==0 ) continue;
+    if( strcmp(zSql,"FAIL")==0){
+      bFail = 1;
+      continue;
+    }
     rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
     if( rc==SQLITE_OK ){
       while( (rc = sqlite3_step(pStmt))==SQLITE_ROW ){
@@ -72,8 +81,18 @@ static void tryEvalFunc(
         bDone = 1;
         break;
       }
+    }else if( bFail ){
+      sqlite3_result_error(context, sqlite3_errmsg(db), -1);
+      sqlite3_result_error_code(context, rc);
+      sqlite3_finalize(pStmt);
+      break;
+    }
+    rc = sqlite3_finalize(pStmt);
+    if( bFail && rc ){
+      sqlite3_result_error(context, sqlite3_errmsg(db), -1);
+      sqlite3_result_error_code(context, rc);
+      break;
     }
-    sqlite3_finalize(pStmt);
   }
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`expire`
- 外部函数：`sqlite3_context_db_handle`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_text`
- 外部函数：`tryeval`
- 大写宏：`API`
- 大写宏：`NULL`
- 大写宏：`SQL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 外部类型：`Also`
- 外部类型：`Apparent`
- 外部类型：`Do`
- 外部类型：`Loop`
- 外部类型：`Result`
- 外部类型：`SQLite`
- 外部类型：`See`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Update`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-aa7ef8d710` → 本草稿移入 `cases/defect/auto-sqlite-aa7ef8d710/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
