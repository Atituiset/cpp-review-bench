# auto-sqlite-6c38dfaa75

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#4042a19a2d8c495fe9a242252ce391bb54770e45](https://github.com/sqlite/sqlite/commit/4042a19a2d8c495fe9a242252ce391bb54770e45) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4042a19a2d8c495fe9a242252ce391bb54770e45 (https://github.com/sqlite/sqlite/commit/4042a19a2d8c495fe9a242252ce391bb54770e45)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 11（原始 PR diff 行 6839；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4042a19a2d8c495fe9a242252ce391bb54770e45 Fix a harmless compiler warning added by [116a220245803639]. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6829,14 +6829,15 @@ static SQLITE_NOINLINE int exprCompareVariable(
     sqlite3VdbeReprepareOnBind(pParse->pVdbe, iVar, 0);
     pL = sqlite3VdbeGetBoundValue(pParse->pReprepare, iVar, SQLITE_AFF_BLOB);
     if( pL ){
+      int eType;
       if( sqlite3_value_type(pL)==SQLITE_TEXT ){
         sqlite3_value_text(pL); /* Make sure the encoding is UTF-8 */
       }
       /* The bound value and the literal must be interchangeable, not
       ** just compare equal. For example, integer 1 and real 1.0 are equal
       ** according to MemCompare(), but are different if TEXT affinity
       ** is applied to them. Similarly, 0.0 and -0.0.  */
-      int eType = sqlite3_value_type(pL);
+      eType = sqlite3_value_type(pL);
       if( eType==sqlite3_value_type(pR) && sqlite3MemCompare(pL, pR, 0)==0 ){
         res = 0;
         if( eType==SQLITE_FLOAT ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3MemCompare`
- 外部函数：`sqlite3VdbeGetBoundValue`
- 外部函数：`sqlite3VdbeReprepareOnBind`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`TEXT`
- 大写宏：`UTF`
- 外部类型：`For`
- 外部类型：`Make`
- 外部类型：`Similarly`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6c38dfaa75` → 本草稿移入 `cases/defect/auto-sqlite-6c38dfaa75/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
