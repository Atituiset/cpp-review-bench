# auto-sqlite-57b68fc58e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#012589ecabb5debd4b9a37e96da027e4f41b8838](https://github.com/sqlite/sqlite/commit/012589ecabb5debd4b9a37e96da027e4f41b8838) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #012589ecabb5debd4b9a37e96da027e4f41b8838 (https://github.com/sqlite/sqlite/commit/012589ecabb5debd4b9a37e96da027e4f41b8838)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 11（原始 PR diff 行 767；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 012589ecabb5debd4b9a37e96da027e4f41b8838 Bring the tree-tracker branch up-to-date with trunk. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -441,6 +441,7 @@ static void diskusedFunc(
   int rc;
   sqlite3_stmt *pStmt;
   int n;
+  char *z;
   sqlite3_int64 ii;
   sqlite3_int64 pgsz;
   sqlite3_int64 nPage;
@@ -764,9 +765,11 @@ static void diskusedFunc(
   sqlite3_str_appendf(s.pOut,
     "The following SQL will create a table named \"space_used\" which\n"
     "contains most of the information used to generate the report above.\n"
-    "*/\n"
   );
+  z = sqlite3_str_value(s.pOut);
+  while( z && (z = strstr(z,"*/"))!=0 ){ z[0] = '+'; }
   sqlite3_str_appendf(s.pOut,
+    "*/\n"
     "BEGIN;\n"
     "CREATE TABLE space_used(\n"
     "   name text,                -- A table or index\n"                /* 0 */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`space_used`
- 外部函数：`sqlite3_str_appendf`
- 大写宏：`BEGIN`
- 大写宏：`CREATE`
- 大写宏：`SQL`
- 大写宏：`TABLE`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-57b68fc58e` → 本草稿移入 `cases/defect/auto-sqlite-57b68fc58e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
