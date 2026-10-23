# auto-sqlite-6172072edd

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
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #31ebfdf4a1483218a7e17499957eca4caabce915 (https://github.com/sqlite/sqlite/commit/31ebfdf4a1483218a7e17499957eca4caabce915)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 31ebfdf4a1483218a7e17499957eca4caabce915 Performance improvements in PRAGMA integrity_check and in materialized CTEs. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -8253,11 +8253,21 @@ int sqlite3Select(
       ** then make the pItem->iCursor be a copy of the ephemeral table that
       ** holds the result of the materialization. tag-select-0484 */
       CteUse *pCteUse = pItem->u2.pCteUse;
+      /* The materialization is computed once per run of the statement or
+      ** trigger program and does not change after that.  So the subroutine
+      ** call and the OP_OpenDup are needed on the first pass only.  The
+      ** OP_NullRow runs on every pass and leaves the duplicate cursor not
+      ** positioned on a row, as OP_OpenDup does. */
+      int addrOnce = sqlite3VdbeAddOp0(v, OP_Once); VdbeCoverage(v);
       sqlite3VdbeAddOp2(v, OP_Gosub, pCteUse->regRtn, pCteUse->addrM9e);
       if( pItem->iCursor!=pCteUse->iCur ){
         sqlite3VdbeAddOp2(v, OP_OpenDup, pItem->iCursor, pCteUse->iCur);
         VdbeComment((v, "%!S", pItem));
       }
+      sqlite3VdbeJumpHere(v, addrOnce);
+      if( pItem->iCursor!=pCteUse->iCur ){
+        sqlite3VdbeAddOp1(v, OP_NullRow, pItem->iCursor);
+      }
       pSub->nSelectRow = pCteUse->nRowEst;
     }else if( (pPrior = isSelfJoinView(pTabList, pItem, 0, i))!=0 ){
       /* This view has already been materialized by a prior entry in
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`pushDownWhereTerms`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3_stricmp`
- 大写宏：`CTE`
- 大写宏：`FROM`
- 外部类型：`CteUse`
- 外部类型：`OP_Gosub`
- 外部类型：`OP_OpenDup`
- 外部类型：`Range`
- 外部类型：`SF_PushDown`
- 外部类型：`Search`
- 外部类型：`Select`
- 外部类型：`SrcItem`
- 外部类型：`SrcList`
- 外部类型：`The`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6172072edd` → 本草稿移入 `cases/defect/auto-sqlite-6172072edd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
