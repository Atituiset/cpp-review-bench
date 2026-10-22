# auto-sqlite-4e31702c7e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#0168a3f32b41cbe0e08f145c0b52882aa7666c6c](https://github.com/sqlite/sqlite/commit/0168a3f32b41cbe0e08f145c0b52882aa7666c6c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 42 |
| 编译错误数（gcc syntax-only） | 26（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0168a3f32b41cbe0e08f145c0b52882aa7666c6c (https://github.com/sqlite/sqlite/commit/0168a3f32b41cbe0e08f145c0b52882aa7666c6c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1176；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0168a3f32b41cbe0e08f145c0b52882aa7666c6c Mark the AND optimization as "not-a-real-0-or-1" so as not to confuse :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1171,9 +1171,12 @@ Expr *sqlite3ExprAnd(Parse *pParse, Expr *pLeft, Expr *pRight){
     if( (f&(EP_OuterON|EP_InnerON|EP_IsFalse|EP_HasFunc))==EP_IsFalse
      && !IN_RENAME_OBJECT
     ){
+      Expr *p;
       sqlite3ExprDeferredDelete(pParse, pLeft);
       sqlite3ExprDeferredDelete(pParse, pRight);
-      return sqlite3ExprInt32(db, 0);
+      p = sqlite3ExprInt32(db, 0);
+      if( p ) p->op2 = 1;
+      return p;
     }else{
       return sqlite3PExpr(pParse, TK_AND, pLeft, pRight);
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`large`
- 外部函数：`memset`
- 外部函数：`sqlite3DbMallocRawNN`
- 外部函数：`sqlite3DbNNFreeNN`
- 外部函数：`sqlite3ErrorMsg`
- 外部函数：`sqlite3ParserAddCleanup`
- 外部函数：`sqlite3SelectDelete`
- 外部函数：`sqlite3WindowDelete`
- 大写宏：`ALWAYS`
- 大写宏：`IN_RENAME_OBJECT`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_LIMIT_EXPR_DEPTH`
- 大写宏：`SQLITE_MAX_EXPR_DEPTH`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_WINDOWFUNC`
- 大写宏：`TK_AND`
- 大写宏：`TK_FUNCTION`
- 大写宏：`TK_INTEGER`
- 大写宏：`TK_SELECT_COLUMN`
- 外部类型：`Avoid`
- 外部类型：`EP_HasFunc`
- 外部类型：`EP_InnerON`
- 外部类型：`EP_IntValue`
- 外部类型：`EP_IsFalse`
- 外部类型：`EP_IsTrue`
- 外部类型：`EP_Leaf`
- 外部类型：`EP_OuterON`
- 外部类型：`EP_Propagate`
- 外部类型：`EP_Static`
- 外部类型：`EP_TokenOnly`
- 外部类型：`EP_WinFunc`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`ExprList_item`
- 外部类型：`Expression`
- 外部类型：`Left`
- 外部类型：`Parse`
- 外部类型：`Parsing`
- 外部类型：`Right`
- 外部类型：`The`
- 外部类型：`is`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-4e31702c7e` → 本草稿移入 `cases/defect/auto-sqlite-4e31702c7e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
