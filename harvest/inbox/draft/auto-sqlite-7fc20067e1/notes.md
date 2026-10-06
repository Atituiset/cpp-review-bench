# auto-sqlite-7fc20067e1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#466e0851d9159ca355faa1ba63eb8cf6d11884e6](https://github.com/sqlite/sqlite/commit/466e0851d9159ca355faa1ba63eb8cf6d11884e6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 83 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #466e0851d9159ca355faa1ba63eb8cf6d11884e6 (https://github.com/sqlite/sqlite/commit/466e0851d9159ca355faa1ba63eb8cf6d11884e6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 10011；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 466e0851d9159ca355faa1ba63eb8cf6d11884e6 When a 3-byte cell on the leaf of an index page, which has a reported size :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -10008,7 +10008,7 @@ int sqlite3BtreeDelete(BtCursor *pCur, u8 flags){
     assert( pTmp!=0 );
     rc = sqlite3PagerWrite(pLeaf->pDbPage);
     if( rc==SQLITE_OK ){
-      rc = insertCell(pPage, iCellIdx, pCell-4, nCell+4, pTmp, n);
+      rc = insertCell(pPage, iCellIdx, pCell-4, nCell+4-(pCell[0]==2), pTmp, n);
     }
     dropCell(pLeaf, pLeaf->nCell-1, nCell, &rc);
     if( rc ) return rc;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`btreeComputeFreeSpace`
- 外部函数：`cellSize`
- 外部函数：`findCell`
- 外部函数：`get2byte`
- 外部函数：`get2byteAligned`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`ptrmapPutOvflPtr`
- 外部函数：`put2byte`
- 外部函数：`put4byte`
- 外部函数：`sqlite3BeginBenignMalloc`
- 外部函数：`sqlite3EndBenignMalloc`
- 外部函数：`sqlite3PagerFilename`
- 外部函数：`sqlite3PagerIswriteable`
- 外部函数：`sqlite3PagerTempSpace`
- 外部函数：`sqlite3PagerWrite`
- 外部函数：`sqlite3ReportError`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`testcase`
- 外部函数：`xCellSize`
- 外部函数：`zero`
- 大写宏：`BTS_FAST_SECURE`
- 大写宏：`CORRUPT_DB`
- 大写宏：`EVIDENCE`
- 大写宏：`MIN`
- 大写宏：`MX_CELL`
- 大写宏：`NEVER`
- 大写宏：`PRAGMA`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_INLINE`
- 大写宏：`SQLITE_MAX_PAGE_SIZE`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_AUTOVACUUM`
- 大写宏：`TH3`
- 外部类型：`Address`
- 外部类型：`Allocate`
- 外部类型：`ArraySize`
- 外部类型：`At`
- 外部类型：`Beginning`
- 外部类型：`Bytes`
- 外部类型：`Check`
- 外部类型：`Comparison`
- 外部类型：`Content`
- 外部类型：`Excess`
- 外部类型：`Find`
- 外部类型：`First`
- 外部类型：`Given`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Insert`
- 外部类型：`Integer`
- 外部类型：`Last`
- 外部类型：`Local`
- 外部类型：`Loop`
- 外部类型：`Make`
- 外部类型：`Max`
- 外部类型：`MemPage`
- 外部类型：`Minimum`
- 外部类型：`New`
- 外部类型：`Number`
- 外部类型：`Offset`
- 外部类型：`Original`
- 外部类型：`Overflows`
- 外部类型：`Overwrite`
- 外部类型：`Page`
- 外部类型：`Pgno`
- 外部类型：`Reduce`
- 外部类型：`Reduction`
- 外部类型：`Remove`
- 外部类型：`Shortcut`
- 外部类型：`Size`
- 外部类型：`Source`
- 外部类型：`Temp`
- 外部类型：`Temporary`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`Update`
- 外部类型：`Used`
- 外部类型：`When`
- 外部类型：`Where`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7fc20067e1` → 本草稿移入 `cases/defect/auto-sqlite-7fc20067e1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
