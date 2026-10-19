# auto-sqlite-5c04128113

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#4e512bf10d4f6a1bc613c4c0679b003d4570c0de](https://github.com/sqlite/sqlite/commit/4e512bf10d4f6a1bc613c4c0679b003d4570c0de) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 27 |
| 编译错误数（gcc syntax-only） | 155（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4e512bf10d4f6a1bc613c4c0679b003d4570c0de (https://github.com/sqlite/sqlite/commit/4e512bf10d4f6a1bc613c4c0679b003d4570c0de)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 820；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4e512bf10d4f6a1bc613c4c0679b003d4570c0de Fix the decimal extension to correctly deal with NULL parameters. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -817,8 +817,10 @@ static void decimalAddFunc(
   Decimal *pA = decimal_new(context, argv[0], 1);
   Decimal *pB = decimal_new(context, argv[1], 1);
   UNUSED_PARAMETER(argc);
-  decimal_add(pA, pB);
-  decimal_result(context, pA);
+  if( pA && pB ){
+    decimal_add(pA, pB);
+    decimal_result(context, pA);
+  }
   decimal_free(pA);
   decimal_free(pB);
 }
@@ -830,7 +832,7 @@ static void decimalSubFunc(
   Decimal *pA = decimal_new(context, argv[0], 1);
   Decimal *pB = decimal_new(context, argv[1], 1);
   UNUSED_PARAMETER(argc);
-  if( pB ){
+  if( pA && pB ){
     pB->sign = !pB->sign;
     decimal_add(pA, pB);
     decimal_result(context, pA);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`isspace`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_result_null`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_double`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`strlen`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_TEXT`
- 外部类型：`Always`
- 外部类型：`At`
- 外部类型：`Construct`
- 外部类型：`Decimal`
- 外部类型：`Exit`
- 外部类型：`Infinity`
- 外部类型：`Multiplier`
- 外部类型：`NaN`
- 外部类型：`Report`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-5c04128113` → 本草稿移入 `cases/defect/auto-sqlite-5c04128113/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
