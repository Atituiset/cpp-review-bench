# auto-sqlite-7e9dd0e2d8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#6a1f950aaeedf781df59b0a531a13774997f76c3](https://github.com/sqlite/sqlite/commit/6a1f950aaeedf781df59b0a531a13774997f76c3) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 96 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6a1f950aaeedf781df59b0a531a13774997f76c3 (https://github.com/sqlite/sqlite/commit/6a1f950aaeedf781df59b0a531a13774997f76c3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1230；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6a1f950aaeedf781df59b0a531a13774997f76c3 When processing immediate FK constraints, do not assume a CASCADE or SET NULL action resolves all constraints if there a :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1007,6 +1007,23 @@ static int isSetNullAction(Parse *pParse, FKey *pFKey){
   return 0;
 }
 
+/*
+** Return true if the child table of FK constraint pFKey has one or
+** more BEFORE triggers that could interfere with the action (either
+** a CASCADE or SET NULL) caused by either a DELETE (if bUpdate==0)
+** or UPDATE (if bUpdate==1) of the parent table. Or false otherwise.
+*/
+static int fkBeforeTriggersExist(
+  Parse *pParse,                  /* Parse context */
+  FKey *pFKey,                    /* Foreign key constraint */
+  int bUpdate                     /* True for ON UPDATE, false ON DELETE */
+){
+  int mask = 0;
+  int op = (!bUpdate && pFKey->aAction[0]==OE_Cascade) ? TK_DELETE : TK_UPDATE;
+  sqlite3TriggersExist(pParse, pFKey->pFrom, op, 0, &mask);
+  return (mask & TRIGGER_BEFORE)!=0;
+}
+
 /*
 ** This function is called when inserting, deleting or updating a row of
 ** table pTab to generate VDBE code to perform foreign key constraint 
@@ -1210,10 +1227,16 @@ void sqlite3FkCheck(
         if( (db->flags & SQLITE_FkNoAction) ) eAction = OE_None;
 
         fkScanChildren(pParse, pSrc, pTab, pIdx, pFKey, aiCol, regOld, 1);
-        /* If this is a deferred FK constraint, or a CASCADE or SET NULL
-        ** action applies, then any foreign key violations caused by
-        ** removing the parent key will be rectified by the action trigger.
-        ** So do not set the "may-abort" flag in this case.
+        /* If this is an immediate FK constraint, then foreign key violations 
+        ** caused by removing the parent key may cause an abort. 
+        **
+        ** Except - if a CASCADE or SET NULL action applies, then any such
+        ** foreign key violations will be rectified by the action trigger. So
+        ** do not set the "may-abort" flag in this case.
+        **
+        ** Except - if there are BEFORE triggers on the child table, then
+        ** one of them may use RAISE(IGNORE) to prevent the SET NULL or
+        ** CASCADE action from taking place. Do set the "may-abort" flag then.
         **
         ** Note 1: If the FK is declared "ON UPDATE CASCADE", then the
         ** may-abort flag will eventually be set on this statement anyway
@@ -1227,7 +1250,10 @@ void sqlite3FkCheck(
         ** child table to fire. In these cases the fk constraint counters
         ** might be set incorrectly if any OP_FkCounter related scans are 
         ** omitted.  */
-        if( !pFKey->isDeferred && eAction!=OE_Cascade && eAction!=OE_SetNull ){
+        if( !pFKey->isDeferred && (
+            (eAction!=OE_Cascade && eAction!=OE_SetNull)
+         || fkBeforeTriggersExist(pParse, pFKey, aChange!=0)
+        )){
           sqlite3MayAbort(pParse);
         }
       }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`sqlite3ColumnColl`
- 外部函数：`sqlite3Expr`
- 外部函数：`sqlite3ExprAddCollateString`
- 外部函数：`sqlite3ExprAlloc`
- 外部函数：`sqlite3ExprAnd`
- 外部函数：`sqlite3ExprAttachSubtrees`
- 外部函数：`sqlite3ExprDelete`
- 外部函数：`sqlite3ExprDup`
- 外部函数：`sqlite3ExprListAppend`
- 外部函数：`sqlite3ExprListDelete`
- 外部函数：`sqlite3ExprSetHeightAndFlags`
- 外部函数：`sqlite3GetVdbe`
- 外部函数：`sqlite3IsNumericAffinity`
- 外部函数：`sqlite3MayAbort`
- 外部函数：`sqlite3PExpr`
- 外部函数：`sqlite3ResolveExprNames`
- 外部函数：`sqlite3StrICmp`
- 外部函数：`sqlite3TableColumnToStorage`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeJumpHereOrPopInst`
- 外部函数：`sqlite3WhereBegin`
- 外部函数：`sqlite3WhereEnd`
- 外部函数：`sqlite3WhereXXX`
- 大写宏：`AND`
- 大写宏：`BLOB`
- 大写宏：`CASCADE`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`NUMERIC`
- 大写宏：`ROWID`
- 大写宏：`SET`
- 大写宏：`SQL`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_AFF_INTEGER`
- 大写宏：`SQLITE_AFF_NUMERIC`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`SQLITE_OMIT_CAST`
- 大写宏：`TEXT`
- 大写宏：`TK_CAST`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_EQ`
- 大写宏：`TK_ID`
- 大写宏：`TK_IN`
- 大写宏：`TK_IS`
- 大写宏：`TK_NE`
- 大写宏：`TK_NOT`
- 大写宏：`TK_REGISTER`
- 大写宏：`TK_UPLUS`
- 大写宏：`UPDATE`
- 大写宏：`VDBE`
- 大写宏：`WHERE`
- 大写宏：`WITHOUT`
- 外部类型：`Address`
- 外部类型：`Amount`
- 外部类型：`At`
- 外部类型：`Child`
- 外部类型：`Clean`
- 外部类型：`Column`
- 外部类型：`Contents`
- 外部类型：`Context`
- 外部类型：`Create`
- 外部类型：`Database`
- 外部类型：`Either`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`FKey`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`Iterator`
- 外部类型：`Map`
- 外部类型：`Name`
- 外部类型：`NameContext`
- 外部类型：`Note`
- 外部类型：`OE_Cascade`
- 外部类型：`OE_None`
- 外部类型：`OE_SetNull`
- 外部类型：`OP_FkCounter`
- 外部类型：`OP_FkIfZero`
- 外部类型：`Omit`
- 外部类型：`Parent`
- 外部类型：`Parse`
- 外部类型：`Parsing`
- 外部类型：`Resolve`
- 外部类型：`SQLITE_FkNoAction`
- 外部类型：`So`
- 外部类型：`SrcList`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Token`
- 外部类型：`Unresolved`
- 外部类型：`Value`
- 外部类型：`Vdbe`
- 外部类型：`WhereInfo`
- 外部类型：`Which`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7e9dd0e2d8` → 本草稿移入 `cases/defect/auto-sqlite-7e9dd0e2d8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
