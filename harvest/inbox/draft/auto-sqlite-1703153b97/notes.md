# auto-sqlite-1703153b97

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#fc96a3c4530bbc66274133d3727a556821dddce6](https://github.com/sqlite/sqlite/commit/fc96a3c4530bbc66274133d3727a556821dddce6) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fc96a3c4530bbc66274133d3727a556821dddce6 (https://github.com/sqlite/sqlite/commit/fc96a3c4530bbc66274133d3727a556821dddce6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 5074；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fc96a3c4530bbc66274133d3727a556821dddce6 Add the tree-tracker optimization. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4756,6 +4756,9 @@ case OP_OpenEphemeral: {     /* ncycle */
         p->apCsr[pOp->p1] = 0;  /* Not required; helps with static analysis */
       }else{
         assert( sqlite3BtreeClosesWithCursor(pCx->ub.pBtx, pCx->uc.pCursor) );
+        if( pOp->opcode==OP_OpenAutoindex ){
+          sqlite3BtreeCursorHintFlags(pCx->uc.pCursor, BTREE_SEEK_EQ);
+        }
       }
     }
   }
@@ -4890,12 +4893,23 @@ case OP_ColumnsUsed: {
 ** If the cursor P1 was opened using the OPFLAG_SEEKEQ flag, then this
 ** opcode will either land on a record that exactly matches the key, or
 ** else it will cause a jump to P2.  When the cursor is OPFLAG_SEEKEQ,
-** this opcode must be followed by an IdxLE opcode with the same arguments.
+** this opcode must be followed by an IdxLE opcode with the same arguments,
+** except that the P2 argument (the jump destination) can be different.
 ** The IdxGT opcode will be skipped if this opcode succeeds, but the
 ** IdxGT opcode will be used on subsequent loop iterations.  The
 ** OPFLAG_SEEKEQ flags is a hint to the btree layer to say that this
 ** is an equality search.
 **
+** The P2 of the IdxLE might be less than the P2 of this opcode.  The P2
+** of the IdxLE will be the location to jump to if there are one or more
+** matching rows and is the location to continue with the next row of the
+** first containing loop.  The P2 of this opcode will be the location to
+** jump to if zero rows of cursor P1 match the key, and that will be the
+** continuation address of the first outer loop that contributes to the
+** key.  The P2 of this opcode might be greater than the P2 of the
+** subsequent IdxLE if the first outer loop that influences the key is
+** outer to the immediately next outer loop.
+**
 ** This opcode leaves the cursor configured to move in forward order,
 ** from the beginning toward the end.  In other words, the cursor is
 ** configured to use Next, not Prev.
@@ -4957,12 +4971,23 @@ case OP_ColumnsUsed: {
 ** If the cursor P1 was opened using the OPFLAG_SEEKEQ flag, then this
 ** opcode will either land on a record that exactly matches the key, or
 ** else it will cause a jump to P2.  When the cursor is OPFLAG_SEEKEQ,
-** this opcode must be followed by an IdxLE opcode with the same arguments.
+** this opcode must be followed by an IdxGE opcode with the same arguments
+** except the P2 of IdxGE might be less than the P2 of this opcode.
 ** The IdxGE opcode will be skipped if this opcode succeeds, but the
 ** IdxGE opcode will be used on subsequent loop iterations.  The
 ** OPFLAG_SEEKEQ flags is a hint to the btree layer to say that this
 ** is an equality search.
 **
+** The P2 of the IdxGE might be less than the P2 of this opcode.  The P2
+** of the IdxGE will be the location to jump to if there are one or more
+** matching rows and is the location to continue with the next row of the
+** first containing loop.  The P2 of this opcode will be the location to
+** jump to if no rows of cursor P1 match the key, and that will be the
+** continuation address of the first outer loop that contributes to the
+** key. The P2 of this opcode might be greater than the P2 of the
+** subsequent IdxLE if the first outer loop that influences the key is
+** outer to the immediately next outer loop.
+**
 ** See also: Found, NotFound, SeekGt, SeekGe, SeekLt
 */
 case OP_SeekLT:         /* jump0, in3, group, ncycle */
@@ -5071,7 +5096,9 @@ case OP_SeekGT: {       /* jump0, in3, group, ncycle */
       assert( pOp->opcode==OP_SeekGE || pOp[1].opcode==OP_IdxLT );
       assert( pOp->opcode==OP_SeekLE || pOp[1].opcode==OP_IdxGT );
       assert( pOp[1].p1==pOp[0].p1 );
-      assert( pOp[1].p2==pOp[0].p2 );
+      assert( pOp[1].p2<=pOp[0].p2 );
+      testcase( pOp->opcode==OP_SeekGE && pOp[1].p2<pOp[0].p2 );
+      testcase( pOp->opcode==OP_SeekLE && pOp[1].p2<pOp[0].p2 );
       assert( pOp[1].p3==pOp[0].p3 );
       assert( pOp[1].p4.i==pOp[0].p4.i );
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3BtreeClosesWithCursor`
- 大写宏：`OPFLAG_SEEKEQ`
- 外部类型：`Found`
- 外部类型：`IdxGE`
- 外部类型：`IdxGT`
- 外部类型：`IdxLE`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Next`
- 外部类型：`Not`
- 外部类型：`NotFound`
- 外部类型：`OP_IdxGT`
- 外部类型：`OP_IdxLT`
- 外部类型：`OP_SeekGE`
- 外部类型：`OP_SeekLE`
- 外部类型：`OP_SeekLT`
- 外部类型：`Prev`
- 外部类型：`See`
- 外部类型：`SeekGe`
- 外部类型：`SeekGt`
- 外部类型：`SeekLt`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`When`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-1703153b97` → 本草稿移入 `cases/defect/auto-sqlite-1703153b97/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
