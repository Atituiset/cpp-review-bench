# auto-sqlite-f8497cf839

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#61a8c1579d1b6721c4f62ea2efda1c218eff36e6](https://github.com/sqlite/sqlite/commit/61a8c1579d1b6721c4f62ea2efda1c218eff36e6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #61a8c1579d1b6721c4f62ea2efda1c218eff36e6 (https://github.com/sqlite/sqlite/commit/61a8c1579d1b6721c4f62ea2efda1c218eff36e6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 593；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 61a8c1579d1b6721c4f62ea2efda1c218eff36e6 The IN pre-filter is now only added when the child key can actually use an :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -543,6 +543,29 @@ static int fkUsesLhsAffinity(Table *p1, int iCol1, Table *p2, int iCol2){
   return (aff1==aff2 || aff1==SQLITE_AFF_BLOB);
 }
 
+#ifndef SQLITE_OMIT_CAST
+/*
+** Return non-zero if a lookup on column iCol of table pTab, using collation
+** sequence zColl, can be done using either the rowid or an index. That is,
+** if iCol is the INTEGER PRIMARY KEY of pTab, or if pTab has an index
+** whose left-most column is iCol and whose collation sequence for that
+** column is zColl. Return zero otherwise.
+*/
+static int fkChildKeyIsIndexed(Table *pTab, int iCol, const char *zColl){
+  Index *pIdx;
+  if( iCol==pTab->iPKey ) return 1;
+  for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
+    if( pIdx->aiColumn[0]==iCol
+     && pIdx->pPartIdxWhere==0
+     && sqlite3StrICmp(pIdx->azColl[0], zColl)==0
+    ){
+      return 1;
+    }
+  }
+  return 0;
+}
+#endif
+
 /*
 ** Return an expression equivalent to (pLeft = pRight), where pLeft is a
 ** parent key value and pRight is an unresolved reference to column
@@ -554,7 +577,7 @@ static int fkUsesLhsAffinity(Table *p1, int iCol1, Table *p2, int iCol2){
 ** If the simple (pLeft = pRight) does not naturally use the affinity of
 ** the parent column, the comparison is coded as (pLeft = +pRight). But
 ** the unary + prevents any index on the child column from being used.
-** So in that case an extra term is added:
+** So in that case, if the child column is indexed, an extra term is added:
 **
 **     (pLeft = +pRight) AND
 **     (pRight COLLATE <coll> IN (+pLeft, +CAST(pLeft AS NUMERIC)))
@@ -590,27 +613,34 @@ static Expr *fkParentChildEq(
 
 #ifndef SQLITE_OMIT_CAST
   {
-    static const Token tNumeric = { "NUMERIC", 7 };
     const char *zColl = sqlite3ColumnColl(&pTab->aCol[iCol]);
-    Expr *pCast;
-    ExprList *pList;
-
-    pCast = sqlite3ExprAlloc(db, TK_CAST, &tNumeric, 0);
-    sqlite3ExprAttachSubtrees(db, pCast, sqlite3ExprDup(db, pLeft, 0), 0);
-    pList = sqlite3ExprListAppend(pParse, 0,
-        sqlite3PExpr(pParse, TK_UPLUS, sqlite3ExprDup(db, pLeft, 0), 0)
-    );
-    pList = sqlite3ExprListAppend(pParse, pList,
-        sqlite3PExpr(pParse, TK_UPLUS, pCast, 0)
-    );
-    pIn = sqlite3PExpr(pParse, TK_IN, sqlite3ExprAddCollateString(pParse,
-          sqlite3ExprDup(db, pRight, 0), zColl ? zColl : sqlite3StrBINARY
-    ), 0);
-    if( pIn ){
-      pIn->x.pList = pList;
-      sqlite3ExprSetHeightAndFlags(pParse, pIn);
-    }else{
-      sqlite3ExprListDelete(db, pList);
+    if( zColl==0 ) zColl = sqlite3StrBINARY;
+
+    /* The IN term is only useful if it can drive a lookup on the child
+    ** table. Omit it otherwise, so that a full scan of the child table
+    ** does not have to evaluate it for every row.  */
+    if( fkChildKeyIsIndexed(pChild, iChildCol, zColl) ){
+      static const Token tNumeric = { "NUMERIC", 7 };
+      Expr *pCast;
+      ExprList *pList;
+
+      pCast = sqlite3ExprAlloc(db, TK_CAST, &tNumeric, 0);
+      sqlite3ExprAttachSubtrees(db, pCast, sqlite3ExprDup(db, pLeft, 0), 0);
+      pList = sqlite3ExprListAppend(pParse, 0,
+          sqlite3PExpr(pParse, TK_UPLUS, sqlite3ExprDup(db, pLeft, 0), 0)
+      );
+      pList = sqlite3ExprListAppend(pParse, pList,
+          sqlite3PExpr(pParse, TK_UPLUS, pCast, 0)
+      );
+      pIn = sqlite3PExpr(pParse, TK_IN, sqlite3ExprAddCollateString(pParse,
+            sqlite3ExprDup(db, pRight, 0), zColl
+      ), 0);
+      if( pIn ){
+        pIn->x.pList = pList;
+        sqlite3ExprSetHeightAndFlags(pParse, pIn);
+      }else{
+        sqlite3ExprListDelete(db, pList);
+      }
     }
   }
 #endif
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`as`
- 外部函数：`simple`
- 外部函数：`sqlite3ColumnColl`
- 外部函数：`sqlite3ExprAddCollateString`
- 外部函数：`sqlite3ExprAlloc`
- 外部函数：`sqlite3ExprAttachSubtrees`
- 外部函数：`sqlite3ExprDup`
- 外部函数：`sqlite3ExprListAppend`
- 外部函数：`sqlite3ExprListDelete`
- 外部函数：`sqlite3ExprSetHeightAndFlags`
- 外部函数：`sqlite3PExpr`
- 外部函数：`to`
- 大写宏：`AND`
- 大写宏：`CAST`
- 大写宏：`COLLATE`
- 大写宏：`NUMERIC`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_OMIT_CAST`
- 大写宏：`TK_CAST`
- 大写宏：`TK_IN`
- 大写宏：`TK_UPLUS`
- 外部类型：`But`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`If`
- 外部类型：`Return`
- 外部类型：`So`
- 外部类型：`Token`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-f8497cf839` → 本草稿移入 `cases/defect/auto-sqlite-f8497cf839/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
