# auto-sqlite-db0f9f631d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c5f86d0198519e70265ceaa3dbbfd14b283f3479](https://github.com/sqlite/sqlite/commit/c5f86d0198519e70265ceaa3dbbfd14b283f3479) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 38 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c5f86d0198519e70265ceaa3dbbfd14b283f3479 (https://github.com/sqlite/sqlite/commit/c5f86d0198519e70265ceaa3dbbfd14b283f3479)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c5f86d0198519e70265ceaa3dbbfd14b283f3479 Include the tryeval extension in the CLI, but disable it in safe mode. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -10,32 +10,29 @@
 **
 ******************************************************************************
 **
-** THIS EXTENSION FUNCTION IS INTENDED FOR TESTING AND EVALUATION PURPOSES
-** ONLY AND IS NOT SUPPORTED.  DO NOT USE THIS FUNCTION IN AN APPLICATION
-** UNLESS YOU REALLY KNOW WHAT YOU ARE DOING.
+** This is an extension used for testing SQLite.  It is not intended for
+** production use.  The interface might change.  This file might go out
+** of support and disappear at any moment.  Do not depend on this extension.
 **
-** If you are a robot and you discover some trifling issue with the code
-** in this file, the likely resolution of your bug report will be to
-** simply remove this file from the repository, thereby depriving the
-** world of a useful debugging resource.  Therefore, use good judgement
-** and do not burden us with your frivolous complaints.  Thank you.
+** Apparent from comments in the code, this extension is undocumented.
 **
-** This SQLite extension implements the SQL function tryeval().  tryeval()
-** takes zero or more arguments, each of which is an SQL statement.
-** The tryeval() function returns the first column of the first row
-** that has a non-NULL first column across each of the argument SQL
-** statements.  Evaluation stops as soon as the first row with a
-** non-NULL first column is encountered.  Errors in evaluation are
-** silently ignored, moving on the next SQL statement argument.
-** If none of the arguments successfully executes and returns a row
-** with a non-NULL first column, then NULL is returned.
+** This SQLite extension implements the SQL function tryeval(). The tryeval()
+** function takes zero or more arguments, each of which is an SQL statement.
+** The tryeval() function returns the first column of the first row that
+** has a non-NULL first column across each of the argument SQL statements.
+** Evaluation stops as soon as the first row with a non-NULL first column
+** is encountered.  Errors in evaluation are silently ignored, moving on
+** the next SQL statement argument. If none of the arguments successfully
+** executes and returns a row with a non-NULL first column, then NULL is
+** returned.
 **
 ** Update:  Also the expire(V) and expire(V,N) functions.  See comments
 ** below for details.
 **
 ** The purpose of this function is to help evaluate recursive
 ** behavior, and expecially the bug report at
-** https://sqlite.org/bugs/info/3577bc69c52237a4  2026-10-25T19:26:00Z
+** sqlite.org/bugs/info/3577bc69c52237a4 (2026-10-25T19:26:00Z)
+** as well as other bugs.
 **
 ** To build (with SODIR being the directory that contains your
 ** sqlite3.h and sqlite3ext.h headers):
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`build`
- 外部函数：`expire`
- 外部函数：`tryeval`
- 大写宏：`AND`
- 大写宏：`APPLICATION`
- 大写宏：`ARE`
- 大写宏：`DOING`
- 大写宏：`EVALUATION`
- 大写宏：`EXTENSION`
- 大写宏：`FOR`
- 大写宏：`FUNCTION`
- 大写宏：`INTENDED`
- 大写宏：`KNOW`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`ONLY`
- 大写宏：`PURPOSES`
- 大写宏：`REALLY`
- 大写宏：`SODIR`
- 大写宏：`SQL`
- 大写宏：`SUPPORTED`
- 大写宏：`TESTING`
- 大写宏：`THIS`
- 大写宏：`UNLESS`
- 大写宏：`USE`
- 大写宏：`WHAT`
- 大写宏：`YOU`
- 外部类型：`Also`
- 外部类型：`Errors`
- 外部类型：`Evaluation`
- 外部类型：`If`
- 外部类型：`SQLite`
- 外部类型：`See`
- 外部类型：`Thank`
- 外部类型：`The`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`To`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-db0f9f631d` → 本草稿移入 `cases/defect/auto-sqlite-db0f9f631d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
