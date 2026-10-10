# auto-sqlite-2f5402bddd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#fc96a3c4530bbc66274133d3727a556821dddce6](https://github.com/sqlite/sqlite/commit/fc96a3c4530bbc66274133d3727a556821dddce6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 66 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fc96a3c4530bbc66274133d3727a556821dddce6 (https://github.com/sqlite/sqlite/commit/fc96a3c4530bbc66274133d3727a556821dddce6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fc96a3c4530bbc66274133d3727a556821dddce6 Add the tree-tracker optimization. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2922,7 +2922,6 @@ int sqlite3ExprIsConstantOrFunction(Expr *p, u8 isInit){
   return exprIsConst(0, p, 4+isInit);
 }
 
-#ifdef SQLITE_ENABLE_CURSOR_HINTS
 /*
 ** Walk an expression tree.  Return 1 if the expression contains a
 ** subquery of some kind.  Return 0 if there are no subqueries.
@@ -2938,7 +2937,6 @@ int sqlite3ExprContainsSubquery(Expr *p){
   sqlite3WalkExpr(&w, p);
   return w.eCode==0;
 }
-#endif
 
 /*
 ** If the expression p codes a constant integer between 0 and 0x7fffffff,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3FindFunction`
- 外部函数：`sqlite3StrICmp`
- 外部函数：`sqlite3WalkExpr`
- 外部函数：`sqlite3WalkExprList`
- 外部函数：`sqlite3_prepare`
- 外部函数：`testcase`
- 大写宏：`CREATE`
- 大写宏：`DEFAULT`
- 大写宏：`ENC`
- 大写宏：`NEVER`
- 大写宏：`NULL`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_ENABLE_CURSOR_HINTS`
- 大写宏：`SQLITE_FUNC_CONSTANT`
- 大写宏：`SQLITE_FUNC_SLOCHNG`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`TABLE`
- 大写宏：`TK_AGG_COLUMN`
- 大写宏：`TK_AGG_FUNCTION`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_DOT`
- 大写宏：`TK_EXISTS`
- 大写宏：`TK_FUNCTION`
- 大写宏：`TK_ID`
- 大写宏：`TK_IF_NULL_ROW`
- 大写宏：`TK_IS`
- 大写宏：`TK_ISNOT`
- 大写宏：`TK_NULL`
- 大写宏：`TK_RAISE`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_SELECT`
- 大写宏：`TK_STRING`
- 大写宏：`TK_TRUEFALSE`
- 大写宏：`TK_VARIABLE`
- 大写宏：`UNUSED_PARAMETER`
- 大写宏：`USING`
- 外部类型：`Convert`
- 外部类型：`EP_ConstFunc`
- 外部类型：`EP_FixedCol`
- 外部类型：`EP_FromDDL`
- 外部类型：`EP_IntValue`
- 外部类型：`EP_IsFalse`
- 外部类型：`EP_IsTrue`
- 外部类型：`EP_OuterON`
- 外部类型：`EP_Quoted`
- 外部类型：`EP_TokenOnly`
- 外部类型：`EP_WinFunc`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`FuncDef`
- 外部类型：`Function`
- 外部类型：`Functions`
- 外部类型：`If`
- 外部类型：`List`
- 外部类型：`NotUsed`
- 外部类型：`Number`
- 外部类型：`Parse`
- 外部类型：`Return`
- 外部类型：`Select`
- 外部类型：`Silently`
- 外部类型：`The`
- 外部类型：`WRC_Abort`
- 外部类型：`WRC_Continue`
- 外部类型：`WRC_Prune`
- 外部类型：`Walk`
- 外部类型：`Walker`
- 外部类型：`Window`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2f5402bddd` → 本草稿移入 `cases/defect/auto-sqlite-2f5402bddd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
