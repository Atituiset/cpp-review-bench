# auto-sqlite-bfccea2e07

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#6590ca855f645a1d163ba99eae4a611acb102f30](https://github.com/sqlite/sqlite/commit/6590ca855f645a1d163ba99eae4a611acb102f30) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 54（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6590ca855f645a1d163ba99eae4a611acb102f30 (https://github.com/sqlite/sqlite/commit/6590ca855f645a1d163ba99eae4a611acb102f30)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 77；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6590ca855f645a1d163ba99eae4a611acb102f30 Avoid applying RealAffinity to a USING column of a FULL JOIN subquery if it is materialized. Report [bugs:/info/2026-10- :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -73,12 +73,17 @@ char sqlite3ExprAffinity(const Expr *pExpr){
           pExpr->pLeft->x.pSelect->pEList->a[pExpr->iColumn].pExpr
       );
     }
-    if( op==TK_VECTOR
-     || (op==TK_FUNCTION && pExpr->affExpr==SQLITE_AFF_DEFER)
-    ){
+    if( op==TK_VECTOR ){
       assert( ExprUseXList(pExpr) );
       return sqlite3ExprAffinity(pExpr->x.pList->a[0].pExpr);
     }
+    if( op==TK_FUNCTION && pExpr->affExpr==SQLITE_AFF_DEFER ){
+      /* The USING column of a FULL JOIN: coalesce(L.x, R.x). Use the
+      ** affinity of L.x, except that REAL becomes NUMERIC, since the
+      ** value may come from R.x and so need not be REAL.  */
+      char aff = sqlite3ExprAffinity(pExpr->x.pList->a[0].pExpr);
+      return aff==SQLITE_AFF_REAL ? SQLITE_AFF_NUMERIC : aff;
+    }
     if( ExprHasProperty(pExpr, EP_Skip|EP_IfNullRow) ){
       assert( pExpr->op==TK_COLLATE
            || pExpr->op==TK_IF_NULL_ROW
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3AffinityType`
- 大写宏：`NEVER`
- 大写宏：`SQLITE_AFF_DEFER`
- 大写宏：`SQLITE_AFF_INTEGER`
- 大写宏：`SQLITE_OMIT_CAST`
- 大写宏：`TK_AGG_COLUMN`
- 大写宏：`TK_CAST`
- 大写宏：`TK_COLLATE`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_FUNCTION`
- 大写宏：`TK_IF_NULL_ROW`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_SELECT`
- 大写宏：`TK_SELECT_COLUMN`
- 大写宏：`TK_VECTOR`
- 外部类型：`EP_IfNullRow`
- 外部类型：`EP_IntValue`
- 外部类型：`EP_Skip`
- 外部类型：`Expr`
- 外部类型：`Table`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-bfccea2e07` → 本草稿移入 `cases/defect/auto-sqlite-bfccea2e07/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
