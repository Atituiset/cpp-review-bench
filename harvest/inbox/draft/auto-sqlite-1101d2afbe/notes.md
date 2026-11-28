# auto-sqlite-1101d2afbe

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#bbe31d3b55e953271106851216fd993e800797ed](https://github.com/sqlite/sqlite/commit/bbe31d3b55e953271106851216fd993e800797ed) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bbe31d3b55e953271106851216fd993e800797ed (https://github.com/sqlite/sqlite/commit/bbe31d3b55e953271106851216fd993e800797ed)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 7944；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bbe31d3b55e953271106851216fd993e800797ed Fix obscure data leaks in PRAGMA secure_delete=on.  Bugs :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6848,6 +6848,10 @@ static int allocateBtreePage(
     if( rc!=SQLITE_OK ){
       releasePage(*ppPage);
       *ppPage = 0;
+    }else if( pBt->btsFlags & BTS_FAST_SECURE ){
+      /* The page may have been loaded from content that an incremental
+      ** vacuum step vacated earlier in this transaction. */
+      memset((*ppPage)->aData, 0, pBt->pageSize);
     }
     TRACE(("ALLOCATE: %u from end of file\n", *pPgno));
   }
@@ -7941,8 +7945,8 @@ static int editPage(
   if( iOld<iNew ){
     int nShift = pageFreeArray(pPg, iOld, iNew-iOld, pCArray);
     if( NEVER(nShift>nCell) ) return SQLITE_CORRUPT_BKPT;
-    memmove(pPg->aCellIdx, &pPg->aCellIdx[nShift*2], nCell*2);
     nCell -= nShift;
+    memmove(pPg->aCellIdx, &pPg->aCellIdx[nShift*2], nCell*2);
   }
   if( iNewEnd < iOldEnd ){
     int nTail = pageFreeArray(pPg, iNewEnd, iOldEnd - iNewEnd, pCArray);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`cachedCellSize`
- 外部函数：`fflush`
- 外部函数：`freeSpace`
- 外部函数：`get2byte`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`put2byte`
- 外部函数：`sqlite3BeginBenignMalloc`
- 外部函数：`sqlite3EndBenignMalloc`
- 外部函数：`sqlite3PagerFilename`
- 外部函数：`sqlite3PagerGetData`
- 外部函数：`sqlite3PagerGetExtra`
- 外部函数：`sqlite3PagerIswriteable`
- 外部函数：`sqlite3PagerUnrefNotNull`
- 外部函数：`sqlite3ReportError`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_mutex_held`
- 大写宏：`ALLOCATE`
- 大写宏：`BTS_FAST_SECURE`
- 大写宏：`CORRUPT_DB`
- 大写宏：`NEVER`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_WITHIN`
- 大写宏：`TH3`
- 外部类型：`Address`
- 外部类型：`Array`
- 外部类型：`At`
- 外部类型：`CellArray`
- 外部类型：`Cells`
- 外部类型：`Check`
- 外部类型：`Find`
- 外部类型：`First`
- 外部类型：`If`
- 外部类型：`Insert`
- 外部类型：`MemPage`
- 外部类型：`Minimum`
- 外部类型：`No`
- 外部类型：`Offset`
- 外部类型：`Original`
- 外部类型：`Overwrite`
- 外部类型：`Page`
- 外部类型：`Reduction`
- 外部类型：`Shortcut`
- 外部类型：`Temporary`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-1101d2afbe` → 本草稿移入 `cases/defect/auto-sqlite-1101d2afbe/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
