# auto-sqlite-4c57520375

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#31ebfdf4a1483218a7e17499957eca4caabce915](https://github.com/sqlite/sqlite/commit/31ebfdf4a1483218a7e17499957eca4caabce915) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #31ebfdf4a1483218a7e17499957eca4caabce915 (https://github.com/sqlite/sqlite/commit/31ebfdf4a1483218a7e17499957eca4caabce915)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1856；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 31ebfdf4a1483218a7e17499957eca4caabce915 Performance improvements in PRAGMA integrity_check and in materialized CTEs. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1850,14 +1850,9 @@ void sqlite3Pragma(
         sqlite3OpenTableAndIndices(pParse, pTab, OP_OpenRead, 0,
                                    1, 0, &iDataCur, &iIdxCur);
         /* reg[7] counts the number of entries in the table.
-        ** reg[8+i] counts the number of entries in the i-th index
         */
         sqlite3VdbeAddOp2(v, OP_Integer, 0, 7);
-        for(j=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, j++){
-          sqlite3VdbeAddOp2(v, OP_Integer, 0, 8+j); /* index entries counter */
-        }
-        assert( pParse->nMem>=8+j );
-        assert( sqlite3NoTempsInRange(pParse,1,7+j) );
+        assert( sqlite3NoTempsInRange(pParse,1,7) );
         sqlite3VdbeAddOp2(v, OP_Rewind, iDataCur, 0); VdbeCoverage(v);
         loopTop = sqlite3VdbeAddOp2(v, OP_AddImm, 7, 1);
 
@@ -2070,7 +2065,6 @@ void sqlite3Pragma(
             r1 = sqlite3GenerateIndexKey(pParse, pIdx, iDataCur, 0, 0, &jmp3,
                                          pPrior, r1);
             pPrior = pIdx;
-            sqlite3VdbeAddOp2(v, OP_AddImm, 8+j, 1);/* increment entry count */
             /* Verify that an index entry exists for the current table row */
             sqlite3VdbeAddOp4Int(v, OP_Found, iIdxCur+j, ckUniq, r1,
                                         pIdx->nColumn); VdbeCoverage(v);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3GenerateIndexKey`
- 外部函数：`sqlite3NoTempsInRange`
- 外部函数：`sqlite3OpenTableAndIndices`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeAddOp4Int`
- 外部类型：`OP_AddImm`
- 外部类型：`OP_Found`
- 外部类型：`OP_Integer`
- 外部类型：`OP_OpenRead`
- 外部类型：`OP_Rewind`
- 外部类型：`Verify`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-4c57520375` → 本草稿移入 `cases/defect/auto-sqlite-4c57520375/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
