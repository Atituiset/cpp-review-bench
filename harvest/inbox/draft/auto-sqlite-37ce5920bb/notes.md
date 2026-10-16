# auto-sqlite-37ce5920bb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c2d796b3f2a277e3dcb8f2905f2908ac1de71cc9](https://github.com/sqlite/sqlite/commit/c2d796b3f2a277e3dcb8f2905f2908ac1de71cc9) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 35 |
| 编译错误数（gcc syntax-only） | 59（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c2d796b3f2a277e3dcb8f2905f2908ac1de71cc9 (https://github.com/sqlite/sqlite/commit/c2d796b3f2a277e3dcb8f2905f2908ac1de71cc9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 382；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c2d796b3f2a277e3dcb8f2905f2908ac1de71cc9 Remove code that is no longer reachable after [bbdd13370de93035].  Add :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -375,11 +375,10 @@ static char comparisonAffinity(const Expr *pExpr){
           pExpr->op==TK_GT || pExpr->op==TK_GE || pExpr->op==TK_LE ||
           pExpr->op==TK_NE || pExpr->op==TK_IS || pExpr->op==TK_ISNOT );
   assert( pExpr->pLeft );
+  assert( !ExprUseXSelect(pExpr) );
   aff = sqlite3ExprAffinity(pExpr->pLeft);
   if( pExpr->pRight ){
     aff = sqlite3CompareAffinity(pExpr->pRight, aff);
-  }else if( ExprUseXSelect(pExpr) ){
-    aff = sqlite3CompareAffinity(pExpr->x.pSelect->pEList->a[0].pExpr, aff);
   }else if( aff==0 ){
     aff = SQLITE_AFF_BLOB;
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3AffinityType`
- 外部函数：`sqlite3IsNumericAffinity`
- 大写宏：`NEVER`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_AFF_DEFER`
- 大写宏：`SQLITE_AFF_INTEGER`
- 大写宏：`SQLITE_AFF_NONE`
- 大写宏：`SQLITE_AFF_NUMERIC`
- 大写宏：`SQLITE_OMIT_CAST`
- 大写宏：`TK_AGG_COLUMN`
- 大写宏：`TK_CAST`
- 大写宏：`TK_COLLATE`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_FUNCTION`
- 大写宏：`TK_GE`
- 大写宏：`TK_GT`
- 大写宏：`TK_IF_NULL_ROW`
- 大写宏：`TK_IS`
- 大写宏：`TK_ISNOT`
- 大写宏：`TK_LE`
- 大写宏：`TK_NE`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_SELECT`
- 大写宏：`TK_SELECT_COLUMN`
- 大写宏：`TK_VECTOR`
- 外部类型：`Both`
- 外部类型：`EP_IfNullRow`
- 外部类型：`EP_IntValue`
- 外部类型：`EP_Skip`
- 外部类型：`Expr`
- 外部类型：`If`
- 外部类型：`One`
- 外部类型：`Otherwise`
- 外部类型：`Table`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-37ce5920bb` → 本草稿移入 `cases/defect/auto-sqlite-37ce5920bb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
