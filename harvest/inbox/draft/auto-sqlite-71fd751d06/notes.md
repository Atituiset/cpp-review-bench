# auto-sqlite-71fd751d06

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c30e731ac37f5a063736565c4829ad6d05b093f7](https://github.com/sqlite/sqlite/commit/c30e731ac37f5a063736565c4829ad6d05b093f7) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c30e731ac37f5a063736565c4829ad6d05b093f7 (https://github.com/sqlite/sqlite/commit/c30e731ac37f5a063736565c4829ad6d05b093f7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 16（原始 PR diff 行 186；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c30e731ac37f5a063736565c4829ad6d05b093f7 Fix issues on UPSERT with a UNIQUE index with duplicate columns. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -151,6 +151,9 @@ int sqlite3UpsertAnalyzeTarget(
     /* Check for matches against other indexes */
     for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
       int ii, jj, nn;
+      int bHit;              /* Index column matches some target term */
+      Bitmask mTarget = 0;   /* Target terms matched by some index column */
+      Bitmask mCol;          /* Target terms matched by index column ii */
       if( !IsUniqueIndex(pIdx) ) continue;
       if( pTarget->nExpr!=pIdx->nKeyCol ) continue;
       if( pIdx->pPartIdxWhere ){
@@ -178,21 +181,34 @@ int sqlite3UpsertAnalyzeTarget(
           sCol[1].iColumn = pIdx->aiColumn[ii];
           pExpr = &sCol[0];
         }
+        mCol = 0;
+        bHit = 0;
         for(jj=0; jj<nn; jj++){
           if( sqlite3ExprCompare(0,pTarget->a[jj].pExpr,pExpr,iCursor)<2 ){
-            break;  /* Column ii of the index matches column jj of target */
+            /* Column ii of the index matches column jj of target */
+            bHit = 1;
+            if( jj<BMS ) mCol |= MASKBIT(jj);
           }
         }
-        if( jj>=nn ){
-          /* The target contains no match for column jj of the index */
+        if( !bHit ){
+          /* The target contains no match for column ii of the index */
           break;
         }
+        mTarget |= mCol;
       }
       if( ii<nn ){
         /* Column ii of the index did not match any term of the conflict target.
         ** Continue the search with the next index. */
         continue;
       }
+      if( nn<BMS && mTarget!=(MASKBIT(nn)-1) ){
+        /* Every column of the index matches a term of the conflict target,
+        ** but some term of the target matches no column of the index.  This
+        ** can happen if the index contains the same column twice, as in
+        ** UNIQUE(b,b) versus a conflict target of (b,c).  Not a match.
+        ** Targets with BMS or more terms are not checked this way. */
+        continue;
+      }
       pUpsert->pUpsertIdx = pIdx;
       if( sqlite3UpsertOfIndex(pAll,pIdx)!=pUpsert ){
         /* Really this should be an error.  The isDup ON CONFLICT clause will
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3ExprCompare`
- 大写宏：`CONFLICT`
- 外部类型：`Check`
- 外部类型：`Column`
- 外部类型：`Continue`
- 外部类型：`Index`
- 外部类型：`Really`
- 外部类型：`The`
- 外部类型：`Upsert`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-71fd751d06` → 本草稿移入 `cases/defect/auto-sqlite-71fd751d06/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
