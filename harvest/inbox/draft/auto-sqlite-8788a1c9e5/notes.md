# auto-sqlite-8788a1c9e5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#fff257d3ed2c2d5f77a052d14e931b260d4828a5](https://github.com/sqlite/sqlite/commit/fff257d3ed2c2d5f77a052d14e931b260d4828a5) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fff257d3ed2c2d5f77a052d14e931b260d4828a5 (https://github.com/sqlite/sqlite/commit/fff257d3ed2c2d5f77a052d14e931b260d4828a5)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 295；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fff257d3ed2c2d5f77a052d14e931b260d4828a5 Improve the error messages emitted by fts5 in some cases. Report [forum:2026-10-13T21:57:58Z | 2026-10-13T21:57:58Z]. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -103,7 +103,22 @@ static int fts5CInstIterInit(
   return rc;
 }
 
-
+/*
+** Load error code rc into the context handle pCtx. Except, if 
+** rc==SQLITE_ERROR and there is an error message in the associated db
+** handle, return that error message as well.
+*/
+static void fts5AuxResultError(sqlite3_context *pCtx, int rc){
+  if( rc==SQLITE_ERROR ){
+    sqlite3 *db = sqlite3_context_db_handle(pCtx);
+    if( sqlite3_errcode(db)==SQLITE_ERROR ){
+      const char *zErr = sqlite3_errmsg(db);
+      sqlite3_result_error(pCtx, zErr, -1);
+      return;
+    }
+  }
+  sqlite3_result_error_code(pCtx, rc);
+}
 
 /*************************************************************************
 ** Start of highlight() implementation.
@@ -292,7 +307,7 @@ static void fts5HighlightFunction(
     sqlite3_free(ctx.zOut);
   }
   if( rc!=SQLITE_OK ){
-    sqlite3_result_error_code(pCtx, rc);
+    fts5AuxResultError(pCtx, rc);
   }
 }
 /*
@@ -586,7 +601,7 @@ static void fts5SnippetFunction(
   if( rc==SQLITE_OK ){
     sqlite3_result_text(pCtx, (const char*)ctx.zOut, -1, SQLITE_TRANSIENT);
   }else{
-    sqlite3_result_error_code(pCtx, rc);
+    fts5AuxResultError(pCtx, rc);
   }
   sqlite3_free(ctx.zOut);
   sqlite3_free(aSeen);
@@ -752,7 +767,7 @@ static void fts5Bm25Function(
     }
     sqlite3_result_double(pCtx, -1.0 * score);
   }else{
-    sqlite3_result_error_code(pCtx, rc);
+    fts5AuxResultError(pCtx, rc);
   }
 }
 
@@ -790,13 +805,13 @@ static void fts5GetLocaleFunction(
 
   iCol = fts5ValueClampedInt(apVal[0]);
   if( iCol<0 || iCol>=pApi->xColumnCount(pFts) ){
-    sqlite3_result_error_code(pCtx, SQLITE_RANGE);
+    fts5AuxResultError(pCtx, SQLITE_RANGE);
     return;
   }
 
   rc = pApi->xColumnLocale(pFts, iCol, &zLocale, &nLocale);
   if( rc!=SQLITE_OK ){
-    sqlite3_result_error_code(pCtx, rc);
+    fts5AuxResultError(pCtx, rc);
     return;
   }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fts5ValueClampedInt`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_result_double`
- 外部函数：`sqlite3_result_error_code`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_value_int64`
- 外部函数：`xColumnCount`
- 外部函数：`xColumnLocale`
- 大写宏：`MAX`
- 大写宏：`MIN`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_RANGE`
- 大写宏：`SQLITE_TRANSIENT`
- 外部类型：`Start`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-8788a1c9e5` → 本草稿移入 `cases/defect/auto-sqlite-8788a1c9e5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
