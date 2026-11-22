# auto-sqlite-35dd3daf13

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d8d097aebb6b7641c76ff982d97038a138b0c132](https://github.com/sqlite/sqlite/commit/d8d097aebb6b7641c76ff982d97038a138b0c132) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 40（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d8d097aebb6b7641c76ff982d97038a138b0c132 (https://github.com/sqlite/sqlite/commit/d8d097aebb6b7641c76ff982d97038a138b0c132)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d8d097aebb6b7641c76ff982d97038a138b0c132 Avoid optimizations that persist (such as Bloom filters) on correlated :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -63,6 +63,19 @@ int sqlite3WhereIsDistinct(WhereInfo *pWInfo){
   return pWInfo->eDistinct;
 }
 
+/*
+** Return true if pWInfo is for a correlated subquery inside of a statement
+** that writes to the database.  Such a subquery runs once for each row
+** and can see rows that earlier runs caused to be written.  Structures
+** built on the first run, such as Bloom filters and automatic indexes,
+** might be out of date by the time a later run uses them.
+*/
+int sqlite3WhereMightRerunAfterWrite(const WhereInfo *pWInfo){
+  if( NEVER(pWInfo->pSelect==0) ) return 0;
+  if( (pWInfo->pSelect->selFlags & SF_Correlated)==0 ) return 0;
+  return DbMaskNonZero(sqlite3ParseToplevel(pWInfo->pParse)->writeMask);
+}
+
 /*
 ** Return the number of ORDER BY terms that are satisfied by the
 ** WHERE clause.  A return of 0 means that the output must be
@@ -7366,6 +7379,7 @@ WhereInfo *sqlite3WhereBegin(
   */
   if( pWInfo->nLevel>=2
    && OptimizationEnabled(db, SQLITE_BloomFilter)
+   && !sqlite3WhereMightRerunAfterWrite(pWInfo)
   ){
     whereCheckIfBloomFilterIsUseful(pWInfo);
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3LogEstToInt`
- 外部函数：`sqlite3WhereLoopBloomable`
- 外部函数：`testcase`
- 大写宏：`JT_LEFT`
- 大写宏：`ORDER`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`WHERE`
- 大写宏：`WHERETRACE`
- 大写宏：`WHERE_BLOOMFILTER`
- 大写宏：`WHERE_COLUMN_EQ`
- 大写宏：`WHERE_IDX_ONLY`
- 大写宏：`WHERE_SELFCULL`
- 外部类型：`Bloom`
- 外部类型：`LogEst`
- 外部类型：`Return`
- 外部类型：`SQLITE_BloomFilter`
- 外部类型：`SrcItem`
- 外部类型：`TF_HasStat1`
- 外部类型：`TF_MaybeReanalyze`
- 外部类型：`Table`
- 外部类型：`WhereInfo`
- 外部类型：`WhereLoop`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-35dd3daf13` → 本草稿移入 `cases/defect/auto-sqlite-35dd3daf13/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
