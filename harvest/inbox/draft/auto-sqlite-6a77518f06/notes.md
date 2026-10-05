# auto-sqlite-6a77518f06

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#52a8a406eb9cda436b10842734c63050da6d8f13](https://github.com/sqlite/sqlite/commit/52a8a406eb9cda436b10842734c63050da6d8f13) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #52a8a406eb9cda436b10842734c63050da6d8f13 (https://github.com/sqlite/sqlite/commit/52a8a406eb9cda436b10842734c63050da6d8f13)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 47（原始 PR diff 行 3731；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 52a8a406eb9cda436b10842734c63050da6d8f13 Merge trunk into this branch. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3728,8 +3728,8 @@ static void fts3SnippetFunc(
   const char *zStart = "<b>";
   const char *zEnd = "</b>";
   const char *zEllipsis = "<b>...</b>";
-  int iCol = -1;
-  int nToken = 15;                /* Default number of tokens in snippet */
+  i64 iCol = -1;
+  i64 nToken = 15;                /* Default number of tokens in snippet */
 
   /* There must be at least one argument passed to this function (otherwise
   ** the non-overloaded version would have been called instead of this one).
@@ -3745,9 +3745,9 @@ static void fts3SnippetFunc(
   pTab = (Fts3Table *)pCsr->base.pVtab;
 
   switch( nVal ){
-    case 6: nToken = sqlite3_value_int(apVal[5]);
+    case 6: nToken = sqlite3_value_int64(apVal[5]);
             /* no break */ deliberate_fall_through
-    case 5: iCol = sqlite3_value_int(apVal[4]);
+    case 5: iCol = sqlite3_value_int64(apVal[4]);
             /* no break */ deliberate_fall_through
     case 4: zEllipsis = (const char*)sqlite3_value_text(apVal[3]);
             /* no break */ deliberate_fall_through
@@ -3760,6 +3760,9 @@ static void fts3SnippetFunc(
   }else if( nToken==0 || iCol>=pTab->nColumn ){
     sqlite3_result_text(pContext, "", -1, SQLITE_STATIC);
   }else if( SQLITE_OK==fts3CursorSeek(pContext, pCsr) ){
+    if( iCol<0 ) iCol = -1;
+    if( nToken<-64 ) nToken = -64;
+    if( nToken>64 ) nToken = 64;
     sqlite3Fts3Snippet(pContext, pCsr, zStart, zEnd, zEllipsis, iCol, nToken);
   }
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`function`
- 外部函数：`sqlite3Fts3PrepareStmt`
- 外部函数：`sqlite3Fts3Snippet`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_result_error_code`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_int`
- 外部函数：`sqlite3_value_text`
- 大写宏：`FTS_CORRUPT_VTAB`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`WHERE`
- 外部类型：`Default`
- 外部类型：`Fts3Cursor`
- 外部类型：`Fts3Table`
- 外部类型：`If`
- 外部类型：`The`
- 外部类型：`There`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6a77518f06` → 本草稿移入 `cases/defect/auto-sqlite-6a77518f06/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
