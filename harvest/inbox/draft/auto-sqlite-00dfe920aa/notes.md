# auto-sqlite-00dfe920aa

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#67897e97219d9cc6dccbcb89fcecd709faa4bc86](https://github.com/sqlite/sqlite/commit/67897e97219d9cc6dccbcb89fcecd709faa4bc86) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 56（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #67897e97219d9cc6dccbcb89fcecd709faa4bc86 (https://github.com/sqlite/sqlite/commit/67897e97219d9cc6dccbcb89fcecd709faa4bc86)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2417；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 67897e97219d9cc6dccbcb89fcecd709faa4bc86 Harden fts5DoSecureDelete() against problems caused by index corruption. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2414,7 +2414,10 @@ static void fts5SegIterNext(
         bNewTerm = 1;
       }
       assert_nc( iOff<pLeaf->szLeaf );
-      if( iOff>pLeaf->szLeaf ){
+      if( iOff>=pLeaf->szLeaf ){
+        /* If iOff==szLeaf then the first rowid offset in the page header
+        ** (or the first term offset) points at the page footer, and
+        ** pIter->iLeafOffset still has its value from the previous page. */
         FTS5_CORRUPT_ITER(p, pIter);
         return;
       }
@@ -5405,7 +5408,10 @@ static void fts5DoSecureDelete(
     }else{
       iStart = fts5GetU16(&aPg[0]);
     }
-    if( iStart>nPg ){
+    if( iStart>=iPgIdx || pSeg->iLeafOffset>iPgIdx ){
+      /* The rowid of the entry being removed, and the start of its position
+      ** list, must be within the leaf content, which ends at szLeaf (iPgIdx).
+      ** The page footer follows. */
       FTS5_CORRUPT_IDX(p);
       sqlite3_free(aIdx);
       return;
@@ -5416,6 +5422,11 @@ static void fts5DoSecureDelete(
 
     if( bDetailNone ){
       int iEnd = pSeg->iLeafOffset - (pSeg->bDel ? 1 + pSeg->nPos : 0);
+      if( iEnd>iPgIdx ){
+        FTS5_CORRUPT_IDX(p);
+        sqlite3_free(aIdx);
+        return;
+      }
       while( iSOP<iEnd ){
         if( aPg[iSOP]==0x00 ) iSOP++;
         if( aPg[iSOP]==0x00 ) iSOP++;
@@ -5431,12 +5442,26 @@ static void fts5DoSecureDelete(
       int nPos = 0;
       iSOP += fts5GetVarint32(&aPg[iSOP], nPos);
       while( iSOP<pSeg->iLeafOffset ){
+        /* nPos is the size of a position-list in bytes, times 2, plus the
+        ** delete flag.  The rowid of the next entry follows it, and must
+        ** come before the position-list of the entry being removed. */
+        if( nPos<0 || (nPos/2)>=pSeg->iLeafOffset-iSOP ){
+          FTS5_CORRUPT_IDX(p);
+          sqlite3_free(aIdx);
+          return;
+        }
         iStart = iSOP + (nPos/2);
         iSOP = iStart + fts5GetVarint(&aPg[iStart], &iDelta);
         iSOP += fts5GetVarint32(&aPg[iSOP], nPos);
       }
-      assert_nc( iSOP==pSeg->iLeafOffset );
-      iNextOff = iSOP + pSeg->nPos;
+      if( iSOP!=pSeg->iLeafOffset || pSeg->nPos<0 ){
+        FTS5_CORRUPT_IDX(p);
+        sqlite3_free(aIdx);
+        return;
+      }
+      /* The position list may extend past the end of this page, but take
+      ** care that iSOP + nPos cannot overflow. */
+      iNextOff = (pSeg->nPos>=iPgIdx-iSOP) ? iPgIdx : iSOP + pSeg->nPos;
     }
   }
 
@@ -5486,6 +5511,11 @@ static void fts5DoSecureDelete(
     if( iNextOff!=iPgIdx ){
       u64 iNextDelta = 0;
       iNextOff += fts5GetVarint(&aPg[iNextOff], &iNextDelta);
+      if( iNextOff>iPgIdx ){
+        FTS5_CORRUPT_IDX(p);
+        sqlite3_free(aIdx);
+        return;
+      }
       iOff += sqlite3Fts5PutVarint(&aPg[iOff], iDelta + iNextDelta);
     }
   }else if( 
@@ -5505,7 +5535,13 @@ static void fts5DoSecureDelete(
       if( (iKeyOff+iVal)>(u32)iStart ) break;
       iKeyOff += iVal;
     }
-    assert_nc( iKey>=1 );
+    if( iKey<1 || iKeyOff<4 || iKeyOff>=iStart ){
+      /* There must be a term before the doclist.  Terms start after the
+      ** 4-byte page header. */
+      FTS5_CORRUPT_IDX(p);
+      sqlite3_free(aIdx);
+      return;
+    }
 
     /* Set iDelKeyOff to the value of the footer entry to remove from 
     ** the page. */
@@ -5520,6 +5556,7 @@ static void fts5DoSecureDelete(
       u64 nSuffix = 0;
       u64 nPrefix2 = 0;
       u64 nSuffix2 = 0;
+      i64 nNeed;
 
       iDelKeyOff = iNextOff;
       iNextOff += fts5GetVarint(&aPg[iNextOff], &nPrefix2);
@@ -5533,11 +5570,20 @@ static void fts5DoSecureDelete(
       nPrefix = MIN(nPrefix, nPrefix2);
       nSuffix = (nPrefix2 + nSuffix2) - nPrefix;
 
-      if( (iKeyOff+nSuffix)>(u64)iPgIdx || (iNextOff+nSuffix2)>(u64)iPgIdx ){
+      /* Compare by subtraction, not addition, since the varints above can
+      ** be any 64-bit value and a sum could wrap around.  After the first
+      ** three tests nPrefix2 and nSuffix2 are small, so the sum in
+
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert_nc`
- 外部函数：`fts5DataXXX`
- 外部函数：`fts5GetU16`
- 外部函数：`fts5GetVarint`
- 外部函数：`fts5GetVarint32`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`sqlite3Fts5ConfigErrmsg`
- 外部函数：`sqlite3Fts5PutVarint`
- 外部函数：`sqlite3_free`
- 外部函数：`structure`
- 大写宏：`AND`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`FTS5_CORRUPT`
- 大写宏：`INSERT`
- 大写宏：`MIN`
- 大写宏：`NULL`
- 大写宏：`SET`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Current`
- 外部类型：`Error`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Hash`
- 外部类型：`Fts5Index`
- 外部类型：`Fts5SegIter`
- 外部类型：`Fts5Structure`
- 外部类型：`Hash`
- 外部类型：`Leaf`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`Replace`
- 外部类型：`Rowid`
- 外部类型：`Set`
- 外部类型：`State`
- 外部类型：`The`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-00dfe920aa` → 本草稿移入 `cases/defect/auto-sqlite-00dfe920aa/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
