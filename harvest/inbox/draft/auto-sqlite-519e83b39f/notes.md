# auto-sqlite-519e83b39f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d54522a5b832422ab893a1e9fa7a99ab314ea4e2](https://github.com/sqlite/sqlite/commit/d54522a5b832422ab893a1e9fa7a99ab314ea4e2) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d54522a5b832422ab893a1e9fa7a99ab314ea4e2 (https://github.com/sqlite/sqlite/commit/d54522a5b832422ab893a1e9fa7a99ab314ea4e2)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 181；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d54522a5b832422ab893a1e9fa7a99ab314ea4e2 Fix expression tree depth calculations for COLLATE. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -173,23 +173,29 @@ int sqlite3ExprDataType(const Expr *pExpr){
 ** and the pExpr parameter is returned unchanged.
 */
 Expr *sqlite3ExprAddCollateToken(
-  const Parse *pParse,     /* Parsing context */
+  Parse *pParse,           /* Parsing context */
   Expr *pExpr,             /* Add the "COLLATE" clause to this expression */
   const Token *pCollName,  /* Name of collating sequence */
   int dequote              /* True to dequote pCollName */
 ){
-  if( pCollName->n>0 ){
+  if( pCollName->n>0 && pParse->nErr==0 ){
     Expr *pNew = sqlite3ExprAlloc(pParse->db, TK_COLLATE, pCollName, dequote);
+    assert( pExpr!=0 );
     if( pNew ){
       pNew->pLeft = pExpr;
       pNew->flags |= EP_Collate|EP_Skip;
+#if SQLITE_MAX_EXPR_DEPTH>0
+      pNew->nHeight = pExpr->nHeight+1;
+      sqlite3ExprCheckHeight(pParse, pNew->nHeight);
+      testcase( pParse->nErr>0 );
+#endif
       pExpr = pNew;
     }
   }
   return pExpr;
 }
 Expr *sqlite3ExprAddCollateString(
-  const Parse *pParse,  /* Parsing context */
+  Parse *pParse,        /* Parsing context */
   Expr *pExpr,          /* Add the "COLLATE" clause to this expression */
   const char *zC        /* The collating sequence name */
 ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`sqlite3DbMallocRawNN`
- 外部函数：`sqlite3DequoteExpr`
- 外部函数：`sqlite3Isquote`
- 大写宏：`COLLATE`
- 大写宏：`NULL`
- 大写宏：`SQLITE_MAX_EXPR_DEPTH`
- 大写宏：`TK_COLLATE`
- 外部类型：`Add`
- 外部类型：`EP_Collate`
- 外部类型：`EP_Skip`
- 外部类型：`Expr`
- 外部类型：`Expression`
- 外部类型：`Handle`
- 外部类型：`Might`
- 外部类型：`Name`
- 外部类型：`Parse`
- 外部类型：`Parsing`
- 外部类型：`The`
- 外部类型：`Token`
- 外部类型：`True`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-519e83b39f` → 本草稿移入 `cases/defect/auto-sqlite-519e83b39f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
