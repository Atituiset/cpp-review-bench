# auto-sqlite-59cdaa4290

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#18e33b9a201bfce0351e8ed4a19ae6b89efd4eb6](https://github.com/sqlite/sqlite/commit/18e33b9a201bfce0351e8ed4a19ae6b89efd4eb6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #18e33b9a201bfce0351e8ed4a19ae6b89efd4eb6 (https://github.com/sqlite/sqlite/commit/18e33b9a201bfce0351e8ed4a19ae6b89efd4eb6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 18e33b9a201bfce0351e8ed4a19ae6b89efd4eb6 When a term is completely deleted from an fts5 index in secure-delete mode, ensure any prefix of it is also removed from :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -378,6 +378,7 @@ struct Fts5Index {
   int nRead;                      /* Total number of blocks read */
 
   sqlite3_stmt *pDeleteFromIdx;
+  sqlite3_stmt *pIdxKeyUpdate;    /* UPDATE %_idx SET term WHERE (segid,pgno) */
 
   sqlite3_stmt *pDataVersion;
   i64 iStructVersion;             /* data_version when pStruct read */
@@ -5201,6 +5202,39 @@ static void fts5SecureDeleteIdxEntry(
   }
 }
 
+/*
+** Execute the SQL statement:
+**
+**    UPDATE %_idx SET term=$pKey WHERE (segid, (pgno/2)) = ($iSegid, $iPgno);
+**
+** This is used when a secure-delete operation removes the first term from
+** a segment leaf page that contains other terms. The %_idx key for the
+** page is a prefix of the removed term, so it is replaced with a key
+** derived from the new first term on the page.
+*/
+static void fts5SecureUpdateIdxEntry(
+  Fts5Index *p,                   /* FTS5 backend object */
+  int iSegid,                     /* Id of segment */
+  int iPgno,                      /* Page number within segment */
+  const u8 *pKey, int nKey        /* New key */
+){
+  assert( iPgno!=1 );
+  if( p->rc==SQLITE_OK && p->pIdxKeyUpdate==0 ){
+    fts5IndexPrepareStmt(p, &p->pIdxKeyUpdate, sqlite3_mprintf(
+        "UPDATE '%q'.'%q_idx' SET term=?1 WHERE (segid, (pgno/2)) = (?2, ?3)",
+        p->pConfig->zDb, p->pConfig->zName
+    ));
+  }
+  if( p->rc==SQLITE_OK ){
+    sqlite3_bind_blob(p->pIdxKeyUpdate, 1, pKey, nKey, SQLITE_STATIC);
+    sqlite3_bind_int(p->pIdxKeyUpdate, 2, iSegid);
+    sqlite3_bind_int(p->pIdxKeyUpdate, 3, iPgno);
+    sqlite3_step(p->pIdxKeyUpdate);
+    p->rc = sqlite3_reset(p->pIdxKeyUpdate);
+    sqlite3_bind_null(p->pIdxKeyUpdate, 1);
+  }
+}
+
 /*
 ** This is called when a secure-delete operation removes a position-list
 ** that overflows onto segment page iPgno of segment pSeg. This function
@@ -5502,6 +5536,22 @@ static void fts5DoSecureDelete(
       if( (iKeyOff+nSuffix)>(u64)iPgIdx || (iNextOff+nSuffix2)>(u64)iPgIdx ){
         FTS5_CORRUPT_IDX(p);
       }else{
+        if( iKey==1 && pSeg->iLeafPgno!=1 
+         && nPrefix2<=(u64)pSeg->term.n && nSuffix2>0
+        ){
+          /* The term being removed is the first on the page, so the %_idx
+          ** key for the page is a prefix of it. Replace the key with a
+          ** prefix of the new first term that is one byte longer than
+          ** the prefix it shares with the removed term. */
+          int nNew = (int)nPrefix2 + 1;
+          u8 *aNew = (u8*)sqlite3Fts5MallocZero(&p->rc, nNew);
+          if( aNew ){
+            memcpy(aNew, pSeg->term.p, nNew-1);
+            aNew[nNew-1] = aPg[iNextOff];
+            fts5SecureUpdateIdxEntry(p, iSegid, pSeg->iLeafPgno, aNew, nNew);
+            sqlite3_free(aNew);
+          }
+        }
         if( iKey!=1 ){
           iOff += sqlite3Fts5PutVarint(&aPg[iOff], nPrefix);
         }
@@ -6991,6 +7041,7 @@ int sqlite3Fts5IndexClose(Fts5Index *p){
     sqlite3_finalize(p->pIdxNextSelect);
     sqlite3_finalize(p->pDataVersion);
     sqlite3_finalize(p->pDeleteFromIdx);
+    sqlite3_finalize(p->pIdxKeyUpdate);
     sqlite3Fts5HashFree(p->pHash);
     sqlite3_free(p->zDataTbl);
     sqlite3_free(p);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fts5DataXXX`
- 外部函数：`sqlite3Fts5ConfigErrmsg`
- 外部函数：`sqlite3Fts5HashFree`
- 外部函数：`sqlite3Fts5PutVarint`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`structure`
- 大写宏：`AND`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`FTS5_CORRUPT`
- 大写宏：`INSERT`
- 大写宏：`NULL`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Current`
- 外部类型：`Error`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Hash`
- 外部类型：`Fts5Index`
- 外部类型：`Fts5Structure`
- 外部类型：`Hash`
- 外部类型：`Leaf`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`Rowid`
- 外部类型：`State`
- 外部类型：`This`
- 外部类型：`Total`
- 外部类型：`Variables`
- 外部类型：`Virtual`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-59cdaa4290` → 本草稿移入 `cases/defect/auto-sqlite-59cdaa4290/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
