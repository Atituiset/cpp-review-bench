# auto-sqlite-c66a3fc4bf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#34615b9c1f2fa513e02911bed20fe0579a9c51cb](https://github.com/sqlite/sqlite/commit/34615b9c1f2fa513e02911bed20fe0579a9c51cb) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #34615b9c1f2fa513e02911bed20fe0579a9c51cb (https://github.com/sqlite/sqlite/commit/34615b9c1f2fa513e02911bed20fe0579a9c51cb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 6374；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 34615b9c1f2fa513e02911bed20fe0579a9c51cb Reset the nullRow flag on the pseudo-cursor on each iteration of the loop :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6347,14 +6347,17 @@ case OP_Rowid: {                 /* out2, ncycle */
   break;
 }
 
-/* Opcode: NullRow P1 * * * *
+/* Opcode: NullRow P1 P2 * * *
 **
-** Move the cursor P1 to a null row.  Any OP_Column operations
+** Move the cursor P1 to a null row if P2 is 0.  Any OP_Column operations
 ** that occur while the cursor is on the null row will always
 ** write a NULL.
 **
 ** If cursor P1 is not previously opened, open it now to a special
 ** pseudo-cursor that always returns NULL for every column.
+**
+** If P2 is not 0, then the nullRow flag on the pseudo-cursor is
+** turned off.
 */
 case OP_NullRow: {
   VdbeCursor *pC;
@@ -6371,7 +6374,7 @@ case OP_NullRow: {
     pC->noReuse = 1;
     pC->uc.pCursor = sqlite3BtreeFakeValidCursor();
   }
-  pC->nullRow = 1;
+  pC->nullRow = (pOp->p2==0);
   pC->cacheStatus = CACHE_STALE;
   if( pC->eCurType==CURTYPE_BTREE ){
     assert( pC->uc.pCursor!=0 );
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3BtreeFakeValidCursor`
- 大写宏：`CACHE_STALE`
- 大写宏：`CURTYPE_BTREE`
- 大写宏：`NULL`
- 外部类型：`Any`
- 外部类型：`If`
- 外部类型：`Move`
- 外部类型：`NullRow`
- 外部类型：`OP_Column`
- 外部类型：`OP_NullRow`
- 外部类型：`Opcode`
- 外部类型：`VdbeCursor`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-c66a3fc4bf` → 本草稿移入 `cases/defect/auto-sqlite-c66a3fc4bf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
