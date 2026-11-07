# auto-sqlite-cc00d8e3d3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cd9c337091af53b1199c6243681cee5ead7d6718](https://github.com/sqlite/sqlite/commit/cd9c337091af53b1199c6243681cee5ead7d6718) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 63 |
| 编译错误数（gcc syntax-only） | 55（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cd9c337091af53b1199c6243681cee5ead7d6718 (https://github.com/sqlite/sqlite/commit/cd9c337091af53b1199c6243681cee5ead7d6718)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 4918；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cd9c337091af53b1199c6243681cee5ead7d6718 Do not substitute a value read from an index on a generated column for an identical expression within the query if the a :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4892,6 +4892,30 @@ int sqlite3ExprCanReturnSubtype(Parse *pParse, Expr *pExpr){
   return w.eCode;
 }
 
+/*
+** Return 0 iff the expression in IndexedExpr object p is identical to
+** expression pExpr, and the affinity is such that an entry read from
+** the index may be substituted for pExpr in any context. Otherwise
+** return non-zero.
+*/
+static int indexedExprCompare(
+  IndexedExpr *p,                 /* Indexed expression */
+  Expr *pExpr,                    /* Expression to compare to p */
+  int iDataCur                    /* iTab argument for ExprCompare() */
+){
+  u8 exprAff;
+  if( sqlite3ExprCompare(0, pExpr, p->pExpr, iDataCur)!=0 ) return 1;
+  assert( p->aff>=SQLITE_AFF_BLOB && p->aff<=SQLITE_AFF_NUMERIC );
+  exprAff = sqlite3ExprAffinity(pExpr);
+  if( (exprAff<=SQLITE_AFF_BLOB && p->aff!=SQLITE_AFF_BLOB)
+   || (exprAff==SQLITE_AFF_TEXT && p->aff!=SQLITE_AFF_TEXT)
+   || (exprAff>=SQLITE_AFF_NUMERIC && p->aff!=SQLITE_AFF_NUMERIC)
+  ){
+    /* Affinity mismatch on a generated column */
+    return 1;
+  }
+  return 0;
+}
 
 /*
 ** Check to see if pExpr is one of the indexed expressions on pParse->pIdxEpr.
@@ -4907,24 +4931,13 @@ static SQLITE_NOINLINE int sqlite3IndexedExprLookup(
   IndexedExpr *p;
   Vdbe *v;
   for(p=pParse->pIdxEpr; p; p=p->pIENext){
-    u8 exprAff;
     int iDataCur = p->iDataCur;
     if( iDataCur<0 ) continue;
     if( pParse->iSelfTab ){
       if( p->iDataCur!=pParse->iSelfTab-1 ) continue;
       iDataCur = -1;
     }
-    if( sqlite3ExprCompare(0, pExpr, p->pExpr, iDataCur)!=0 ) continue;
-    assert( p->aff>=SQLITE_AFF_BLOB && p->aff<=SQLITE_AFF_NUMERIC );
-    exprAff = sqlite3ExprAffinity(pExpr);
-    if( (exprAff<=SQLITE_AFF_BLOB && p->aff!=SQLITE_AFF_BLOB)
-     || (exprAff==SQLITE_AFF_TEXT && p->aff!=SQLITE_AFF_TEXT)
-     || (exprAff>=SQLITE_AFF_NUMERIC && p->aff!=SQLITE_AFF_NUMERIC)
-    ){
-      /* Affinity mismatch on a generated column */
-      continue;
-    }
-
+    if( indexedExprCompare(p, pExpr, iDataCur)!=0 ) continue;
 
     /* Functions that might set a subtype should not be replaced by the
     ** value taken from an expression index if they are themselves an
@@ -7570,7 +7583,7 @@ static int analyzeAggregate(Walker *pWalker, Expr *pExpr){
       for(pIEpr=pParse->pIdxEpr; pIEpr; pIEpr=pIEpr->pIENext){
         int iDataCur = pIEpr->iDataCur;
         if( iDataCur<0 ) continue;
-        if( sqlite3ExprCompare(0, pExpr, pIEpr->pExpr, iDataCur)==0 ) break;
+        if( indexedExprCompare(pIEpr, pExpr, iDataCur)==0 ) break;
       }
       if( pIEpr==0 ) break;
       if( NEVER(!ExprUseYTab(pExpr)) ) break;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3AffinityType`
- 外部函数：`sqlite3MemCompare`
- 外部函数：`sqlite3StrICmp`
- 外部函数：`sqlite3ValueFree`
- 外部函数：`sqlite3ValueFromExpr`
- 外部函数：`sqlite3VdbeGetBoundValue`
- 外部函数：`sqlite3VdbeReprepareOnBind`
- 外部函数：`sqlite3WindowCompare`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`strcmp`
- 大写宏：`ALWAYS`
- 大写宏：`NEVER`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_AFF_DEFER`
- 大写宏：`SQLITE_AFF_INTEGER`
- 大写宏：`SQLITE_AFF_NUMERIC`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_OMIT_CAST`
- 大写宏：`SQLITE_OMIT_WINDOWFUNC`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`SQLITE_UTF8`
- 大写宏：`TK_AGG_COLUMN`
- 大写宏：`TK_AGG_FUNCTION`
- 大写宏：`TK_CAST`
- 大写宏：`TK_COLLATE`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_FUNCTION`
- 大写宏：`TK_IF_NULL_ROW`
- 大写宏：`TK_NULL`
- 大写宏：`TK_RAISE`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_SELECT`
- 大写宏：`TK_SELECT_COLUMN`
- 大写宏：`TK_VARIABLE`
- 大写宏：`TK_VECTOR`
- 大写宏：`UTF`
- 外部类型：`Affinity`
- 外部类型：`Check`
- 外部类型：`EP_Commuted`
- 外部类型：`EP_Distinct`
- 外部类型：`EP_FixedCol`
- 外部类型：`EP_IfNullRow`
- 外部类型：`EP_IntValue`
- 外部类型：`EP_Skip`
- 外部类型：`EP_TokenOnly`
- 外部类型：`EP_WinFunc`
- 外部类型：`EP_xIsSelect`
- 外部类型：`Expr`
- 外部类型：`Functions`
- 外部类型：`IndexedExpr`
- 外部类型：`Make`
- 外部类型：`Parse`
- 外部类型：`SQLITE_EnableQPSG`
- 外部类型：`Table`
- 外部类型：`Vdbe`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-cc00d8e3d3` → 本草稿移入 `cases/defect/auto-sqlite-cc00d8e3d3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
