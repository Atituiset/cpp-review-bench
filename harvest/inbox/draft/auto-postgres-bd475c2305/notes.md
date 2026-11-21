# auto-postgres-bd475c2305

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ecb260853526019a1340ba8a6b4c6de27a07d69b](https://github.com/postgres/postgres/commit/ecb260853526019a1340ba8a6b4c6de27a07d69b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ecb260853526019a1340ba8a6b4c6de27a07d69b (https://github.com/postgres/postgres/commit/ecb260853526019a1340ba8a6b4c6de27a07d69b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 479；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ecb260853526019a1340ba8a6b4c6de27a07d69b Reduce WAL generated when copying relation storage :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -476,7 +476,7 @@ RelationPreTruncate(Relation rel)
  */
 void
 RelationCopyStorage(SMgrRelation src, SMgrRelation dst,
-					ForkNumber forkNum, char relpersistence)
+					ForkNumber forkNum, char relpersistence, bool page_std)
 {
 	bool		use_wal;
 	bool		copying_initfork;
@@ -508,6 +508,7 @@ RelationCopyStorage(SMgrRelation src, SMgrRelation dst,
 	for (blkno = 0; blkno < nblocks; blkno++)
 	{
 		BulkWriteBuffer buf;
+		bool		allow_page_std = page_std;
 		int			piv_flags;
 		bool		checksum_failure;
 		bool		verified;
@@ -551,11 +552,20 @@ RelationCopyStorage(SMgrRelation src, SMgrRelation dst,
 		}
 
 		/*
-		 * Queue the page for WAL-logging and writing out.  Unfortunately we
-		 * don't know what kind of a page this is, so we have to log the full
-		 * page including any unused space.
+		 * For page layouts we don't recognize, hole removal is still safe if
+		 * the exact range that WAL would omit contains only zeroes.
 		 */
-		smgr_bulk_write(bulkstate, blkno, buf, false);
+		if (use_wal && !allow_page_std &&
+			PageFreeSpaceBoundsAreValid((Page) buf))
+		{
+			PageHeader	phdr = (PageHeader) buf;
+
+			allow_page_std =
+				pg_memory_is_all_zeros((char *) buf + phdr->pd_lower,
+									   phdr->pd_upper - phdr->pd_lower);
+		}
+
+		smgr_bulk_write(bulkstate, blkno, buf, allow_page_std);
 	}
 	smgr_bulk_finish(bulkstate);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`smgr_bulk_finish`
- 外部函数：`smgr_bulk_write`
- 大写宏：`WAL`
- 外部类型：`BulkWriteBuffer`
- 外部类型：`ForkNumber`
- 外部类型：`Queue`
- 外部类型：`SMgrRelation`
- 外部类型：`Unfortunately`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-bd475c2305` → 本草稿移入 `cases/defect/auto-postgres-bd475c2305/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
