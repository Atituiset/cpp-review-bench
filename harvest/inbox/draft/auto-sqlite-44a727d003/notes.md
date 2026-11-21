# auto-sqlite-44a727d003

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#eef1cf4591ff8b4f43e72aceafb7cf3723a74275](https://github.com/sqlite/sqlite/commit/eef1cf4591ff8b4f43e72aceafb7cf3723a74275) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eef1cf4591ff8b4f43e72aceafb7cf3723a74275 (https://github.com/sqlite/sqlite/commit/eef1cf4591ff8b4f43e72aceafb7cf3723a74275)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 27（原始 PR diff 行 4362；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eef1cf4591ff8b4f43e72aceafb7cf3723a74275 Fix signed integer overflow issues in FTS3. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4309,6 +4309,13 @@ static int fts3IncrmergeLoad(
       iStart = sqlite3_column_int64(pSelect, 1);
       iLeafEnd = sqlite3_column_int64(pSelect, 2);
       fts3ReadEndBlockField(pSelect, 3, &iEnd, &pWriter->nLeafData);
+      if( pWriter->nLeafData==SMALLEST_INT64 ){
+        /* The value cannot be negated, here or by the caller. The record
+        ** is corrupt. */
+        pWriter->nLeafData = 0;
+        sqlite3_reset(pSelect);
+        return FTS_CORRUPT_VTAB;
+      }
       if( pWriter->nLeafData<0 ){
         pWriter->nLeafData = pWriter->nLeafData * -1;
       }
@@ -4359,7 +4366,14 @@ static int fts3IncrmergeLoad(
         return FTS_CORRUPT_VTAB;
       }
 
-      pWriter->nLeafEst = (int)(((iEnd - iStart)+1)/FTS_MAX_APPENDABLE_HEIGHT);
+      /* Block numbers are never negative and iEnd is never less than iStart.
+      ** Check this so that the arithmetic below cannot overflow. */
+      if( iStart<0 || iEnd<iStart || iEnd==LARGEST_INT64 ){
+        sqlite3_reset(pSelect);
+        return FTS_CORRUPT_VTAB;
+      }
+
+      pWriter->nLeafEst = ((iEnd - iStart)+1)/FTS_MAX_APPENDABLE_HEIGHT;
       pWriter->iStart = iStart;
       pWriter->iEnd = iEnd;
       pWriter->iAbsLevel = iAbsLevel;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_column_text`
- 大写宏：`FTS_CORRUPT_VTAB`
- 大写宏：`SMALLEST_INT64`
- 外部类型：`The`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-44a727d003` → 本草稿移入 `cases/defect/auto-sqlite-44a727d003/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
