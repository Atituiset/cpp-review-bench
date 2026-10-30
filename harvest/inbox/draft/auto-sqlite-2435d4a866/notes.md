# auto-sqlite-2435d4a866

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#67914d1478f9f71a1b731b7de76a92fe2cd71129](https://github.com/sqlite/sqlite/commit/67914d1478f9f71a1b731b7de76a92fe2cd71129) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 35（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #67914d1478f9f71a1b731b7de76a92fe2cd71129 (https://github.com/sqlite/sqlite/commit/67914d1478f9f71a1b731b7de76a92fe2cd71129)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 83（原始 PR diff 行 184；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 67914d1478f9f71a1b731b7de76a92fe2cd71129 Fix a possible infinite loop associated with the nocase collation :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -160,38 +160,72 @@ static int readUtf8(const unsigned char *z, unsigned *pOut){
 /*
 ** The nextCharContext structure has been set up.  Add all "next" characters
 ** to the result set.
+**
+** Each step of the loop finds the smallest entry, in collation order, that
+** is greater than both:
+**
+**    ?1 || ?2     where ?2 is c||char(0x10ffff) and c is the most recent
+**                 next character found.  This skips over all other entries
+**                 that continue with c.
+**
+**    ?3           the entry found by the previous step.
+**
+** The ?3 constraint guarantees forward progress, even for collating
+** sequences, such as NOCASE, in which ?1||?2 might sort before entries
+** that have already been visited.
 */
 static void findNextChars(nextCharContext *p){
-  unsigned cPrev = 0;
   unsigned char zPrev[8];
-  int n, rc;
-  
+  int nPrev = 0;
+  unsigned char *zLast = 0;
+  int nLast = 0;
+  int rc;
+
   for(;;){
     sqlite3_bind_text(p->pStmt, 1, (char*)p->zPrefix, p->nPrefix,
                       SQLITE_STATIC);
-    n = writeUtf8(zPrev, cPrev+1);
-    sqlite3_bind_text(p->pStmt, 2, (char*)zPrev, n, SQLITE_STATIC);
+    sqlite3_bind_text(p->pStmt, 2, (char*)zPrev, nPrev, SQLITE_STATIC);
+    if( zLast ){
+      sqlite3_bind_text(p->pStmt, 3, (char*)zLast, nLast, SQLITE_STATIC);
+    }else{
+      sqlite3_bind_text(p->pStmt, 3, (char*)p->zPrefix, p->nPrefix,
+                        SQLITE_STATIC);
+    }
     rc = sqlite3_step(p->pStmt);
     if( rc==SQLITE_DONE ){
-      sqlite3_reset(p->pStmt);
-      return;
+      break;
     }else if( rc!=SQLITE_ROW ){
       p->otherError = rc;
-      return;
+      break;
     }else{
       const unsigned char *zOut = sqlite3_column_text(p->pStmt, 0);
-      unsigned cNext;
-      if( sqlite3_column_bytes(p->pStmt,0)<=p->nPrefix ){
-        sqlite3_reset(p->pStmt);
-        continue;
+      int nOut = sqlite3_column_bytes(p->pStmt, 0);
+      unsigned char *zNew;
+      if( zOut==0 ){
+        p->mallocFailed = 1;
+        break;
+      }
+      zNew = sqlite3_realloc64(zLast, nOut+1);
+      if( zNew==0 ){
+        p->mallocFailed = 1;
+        break;
       }
-      n = readUtf8(zOut+p->nPrefix, &cNext);
+      zLast = zNew;
+      nLast = nOut;
+      memcpy(zLast, zOut, nOut+1);
       sqlite3_reset(p->pStmt);
-      nextCharAppend(p, cNext);
-      cPrev = cNext;
-      if( p->mallocFailed ) return;
+      if( nLast>p->nPrefix ){
+        unsigned cNext;
+        readUtf8(zLast+p->nPrefix, &cNext);
+        nextCharAppend(p, cNext);
+        if( p->mallocFailed ) break;
+        nPrev = writeUtf8(zPrev, cNext);
+        nPrev += writeUtf8(zPrev+nPrev, 0x10ffff);
+      }
     }
   }
+  sqlite3_reset(p->pStmt);
+  sqlite3_free(zLast);
 }
 
 
@@ -250,11 +284,13 @@ static void nextCharFunc(
   }
   zSql = sqlite3_mprintf(
     "SELECT %s FROM %s"
-    " WHERE %s>=(?1 || ?2) %s"
+    " WHERE %s>(?1 || ?2) %s"
+    "   AND +%s>?3 %s"
     "   AND %s<=(?1 || char(1114111)) %s" /* 1114111 == 0x10ffff */
     "   %s"
     " ORDER BY 1 %s ASC LIMIT 1",
-    zField, zTable, zField, zColl, zField, zColl, zWhereClause, zColl
+    zField, zTable, zField, zColl, zField, zColl, zField, zColl,
+    zWhereClause, zColl
   );
   if( zWhereClause[0] ) sqlite3_free(zWhereClause);
   if( zColl[0] ) sqlite3_free(zColl);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`char`
- 外部函数：`sqlite3_bind_text`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 大写宏：`AND`
- 大写宏：`ASC`
- 大写宏：`FROM`
- 大写宏：`LIMIT`
- 大写宏：`ORDER`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_DONE`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`WHERE`
- 外部类型：`Add`
- 外部类型：`The`
- 外部类型：`nextCharContext`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2435d4a866` → 本草稿移入 `cases/defect/auto-sqlite-2435d4a866/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
