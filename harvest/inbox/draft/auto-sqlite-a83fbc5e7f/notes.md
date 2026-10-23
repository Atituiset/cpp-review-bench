# auto-sqlite-a83fbc5e7f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#20822819adf299e3a7609a26206bf2b43ac402ff](https://github.com/sqlite/sqlite/commit/20822819adf299e3a7609a26206bf2b43ac402ff) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 53 |
| 编译错误数（gcc syntax-only） | 32（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #20822819adf299e3a7609a26206bf2b43ac402ff (https://github.com/sqlite/sqlite/commit/20822819adf299e3a7609a26206bf2b43ac402ff)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1392；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 20822819adf299e3a7609a26206bf2b43ac402ff Fix two minor issues with the geopoly extension. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1389,9 +1389,13 @@ static int geopolyFilter(
     /* Special case - lookup by rowid. */
     RtreeNode *pLeaf;        /* Leaf on which the required cell resides */
     RtreeSearchPoint *p;     /* Search point for the leaf */
-    i64 iRowid = sqlite3_value_int64(argv[0]);
+    i64 iRowid;
     i64 iNode = 0;
-    rc = findLeafNode(pRtree, iRowid, &pLeaf, &iNode);
+    if( rtreeRowidEqValue(argv[0], &iRowid) ){
+      rc = findLeafNode(pRtree, iRowid, &pLeaf, &iNode);
+    }else{
+      pLeaf = 0;
+    }
     if( rc==SQLITE_OK && pLeaf!=0 ){
       p = rtreeSearchPointNew(pCsr, RTREE_ZERO, 0);
       assert( p!=0 );  /* Always returns pCsr->sPoint */
@@ -1621,6 +1625,8 @@ static int geopolyUpdate(
   i64 newRowid;                   /* The new rowid */
   int newRowidValid;              /* True if newRowid is valid */
   int coordChange = 0;            /* Change in coordinates */
+  sqlite3_value *pShape = 0;      /* New _shape value */
+  sqlite3_value *pOldShape = 0;   /* Copy of the unchanged _shape */
 
   if( pRtree->nNodeRef ){
     /* Unable to write to the btree while another cursor is reading from it,
@@ -1637,13 +1643,38 @@ static int geopolyUpdate(
   newRowid = newRowidValid ? sqlite3_value_int64(aData[1]) : 0;
   cell.iRowid = newRowid;
 
+  if( nData>1 ){
+    pShape = aData[2];
+    if( oldRowidValid && oldRowid!=newRowid && sqlite3_value_nochange(pShape) ){
+      /* The rowid changes but _shape does not. aData[2] holds no value in
+      ** this case, so read the current _shape from the %_rowid table. It
+      ** is needed for the bounding box of the new r-tree entry and for the
+      ** %_rowid record of the new rowid. */
+      sqlite3_stmt *pRead = 0;
+      rc = sqlite3_prepare_v3(pRtree->db, pRtree->zReadAuxSql, -1, 0,
+                              &pRead, 0);
+      if( rc==SQLITE_OK ){
+        int rc2;
+        sqlite3_bind_int64(pRead, 1, oldRowid);
+        if( sqlite3_step(pRead)==SQLITE_ROW ){
+          pOldShape = sqlite3_value_dup(sqlite3_column_value(pRead, 2));
+          if( pOldShape==0 ) rc = SQLITE_NOMEM;
+          pShape = pOldShape;
+        }
+        rc2 = sqlite3_finalize(pRead);
+        if( rc==SQLITE_OK ) rc = rc2;
+      }
+      if( rc ) goto geopoly_update_end;
+    }
+  }
+
   if( nData>1                                 /* not a DELETE */
    && (!oldRowidValid                         /* INSERT */
-        || !sqlite3_value_nochange(aData[2])  /* UPDATE _shape */
+        || !sqlite3_value_nochange(pShape)    /* UPDATE _shape */
         || oldRowid!=newRowid)                /* Rowid change */
   ){
-    assert( aData[2]!=0 );
-    geopolyBBox(0, aData[2], cell.aCoord, &rc);
+    assert( pShape!=0 );
+    geopolyBBox(0, pShape, cell.aCoord, &rc);
     if( rc ){
       if( rc==SQLITE_ERROR ){
         pVtab->zErrMsg =
@@ -1709,17 +1740,17 @@ static int geopolyUpdate(
     int nChange = 0;
     sqlite3_bind_int64(pUp, 1, cell.iRowid);
     assert( pRtree->nAux>=1 );
-    if( sqlite3_value_nochange(aData[2]) ){
+    if( sqlite3_value_nochange(pShape) ){
       sqlite3_bind_null(pUp, 2);
     }else{
       GeoPoly *p = 0;
-      if( sqlite3_value_type(aData[2])==SQLITE_TEXT
-       && (p = geopolyFuncParam(0, aData[2], &rc))!=0
+      if( sqlite3_value_type(pShape)==SQLITE_TEXT
+       && (p = geopolyFuncParam(0, pShape, &rc))!=0
        && rc==SQLITE_OK
       ){
         sqlite3_bind_blob(pUp, 2, p->hdr, 4+8*p->nVertex, SQLITE_TRANSIENT);
       }else{
-        sqlite3_bind_value(pUp, 2, aData[2]);
+        sqlite3_bind_value(pUp, 2, pShape);
       }
       sqlite3_free(p);
       nChange = 1;
@@ -1735,6 +1766,7 @@ static int geopolyUpdate(
   }
 
 geopoly_update_end:
+  sqlite3_value_free(pOldShape);
   rtreeRelease(pRtree);
   return rc;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`atof`
- 外部函数：`fast_isspace`
- 外部函数：`findLeafNode`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`rtreeRelease`
- 外部函数：`rtreeSearchPointNew`
- 外部函数：`safe_isdigit`
- 外部函数：`sqlite3AtoF`
- 外部函数：`sqlite3Isdigit`
- 外部函数：`sqlite3_bind_blob`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_bind_null`
- 外部函数：`sqlite3_bind_value`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_int64`
- 外部函数：`sqlite3_value_nochange`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`testcase`
- 大写宏：`DELETE`
- 大写宏：`INSERT`
- 大写宏：`RTREE_ZERO`
- 大写宏：`SQLITE_AMALGAMATION`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`SQLITE_TRANSIENT`
- 大写宏：`UPDATE`
- 外部类型：`Always`
- 外部类型：`Change`
- 外部类型：`Context`
- 外部类型：`Error`
- 外部类型：`For`
- 外部类型：`GeoParse`
- 外部类型：`GeoPoly`
- 外部类型：`Leaf`
- 外部类型：`Remove`
- 外部类型：`Results`
- 外部类型：`Rowid`
- 外部类型：`RtreeCoord`
- 外部类型：`RtreeNode`
- 外部类型：`RtreeSearchPoint`
- 外部类型：`Search`
- 外部类型：`Special`
- 外部类型：`The`
- 外部类型：`True`
- 外部类型：`Unable`
- 外部类型：`Write`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-a83fbc5e7f` → 本草稿移入 `cases/defect/auto-sqlite-a83fbc5e7f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
