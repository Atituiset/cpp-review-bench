# auto-postgres-b7848d83ed

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#173cf85a079516fdd2b09ed853702b05fbfcd15e](https://github.com/postgres/postgres/commit/173cf85a079516fdd2b09ed853702b05fbfcd15e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 75 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #173cf85a079516fdd2b09ed853702b05fbfcd15e (https://github.com/postgres/postgres/commit/173cf85a079516fdd2b09ed853702b05fbfcd15e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 173cf85a079516fdd2b09ed853702b05fbfcd15e Fix hash index scans that start during a bucket split. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -35,6 +35,7 @@
 #include "port/pg_bitutils.h"
 #include "storage/predicate.h"
 #include "storage/smgr.h"
+#include "utils/injection_point.h"
 #include "utils/rel.h"
 
 static bool _hash_alloc_buckets(Relation rel, BlockNumber firstblock,
@@ -1109,6 +1110,8 @@ _hash_splitbucket(Relation rel,
 						   BufferGetBlockNumber(bucket_obuf),
 						   BufferGetBlockNumber(bucket_nbuf));
 
+	INJECTION_POINT("hash-split-before-relocation", NULL);
+
 	/*
 	 * Partition the tuples in the old bucket between the old bucket and the
 	 * new bucket, advancing along the old bucket's overflow bucket chain and
@@ -1457,6 +1460,8 @@ _hash_finish_split(Relation rel, Buffer metabuf, Buffer obuf, Bucket obucket,
 	npageopaque = HashPageGetOpaque(npage);
 	nbucket = npageopaque->hasho_bucket;
 
+	INJECTION_POINT("hash-finish-incomplete-split", NULL);
+
 	_hash_splitbucket(rel, metabuf, obucket,
 					  nbucket, obuf, bucket_nbuf, tidhtab,
 					  maxbucket, highmask, lowmask);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_hash_addovflpage`
- 外部函数：`_hash_checkpage`
- 外部函数：`_hash_get_indextuple_hashkey`
- 外部函数：`_hash_getbuf`
- 外部函数：`_hash_hashkey2bucket`
- 外部函数：`_hash_pgaddmultitup`
- 外部函数：`elog`
- 外部函数：`hash_search`
- 外部函数：`key`
- 外部函数：`log_newpage`
- 外部函数：`log_split_page`
- 外部函数：`pfree`
- 外部函数：`smgrextend`
- 大写宏：`BLCKSZ`
- 大写宏：`BUFFER_LOCK_UNLOCK`
- 大写宏：`END_CRIT_SECTION`
- 大写宏：`ERROR`
- 大写宏：`HASHO_PAGE_ID`
- 大写宏：`HASH_FIND`
- 大写宏：`HASH_NOLOCK`
- 大写宏：`HTAB`
- 大写宏：`INDEX_MOVED_BY_SPLIT_MASK`
- 大写宏：`LH_UNUSED_PAGE`
- 大写宏：`MAIN_FORKNUM`
- 大写宏：`MAXALIGN`
- 大写宏：`P_NEW`
- 大写宏：`START_CRIT_SECTION`
- 外部类型：`Before`
- 外部类型：`BlockNumber`
- 外部类型：`Bucket`
- 外部类型：`Buffer`
- 外部类型：`Change`
- 外部类型：`Check`
- 外部类型：`Copy`
- 外部类型：`FirstOffsetNumber`
- 外部类型：`HashPageOpaque`
- 外部类型：`HashPageOpaqueData`
- 外部类型：`IndexTuple`
- 外部类型：`Initialize`
- 外部类型：`InvalidBlockNumber`
- 外部类型：`InvalidBucket`
- 外部类型：`Just`
- 外部类型：`MaxIndexTuplesPerPage`
- 外部类型：`OffsetNumber`
- 外部类型：`Outer`
- 外部类型：`PGIOAlignedBlock`
- 外部类型：`Page`
- 外部类型：`Partition`
- 外部类型：`Relation`
- 外部类型：`Scan`
- 外部类型：`Size`
- 外部类型：`TIDs`
- 外部类型：`We`
- 外部类型：`XLogRecPtr`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b7848d83ed` → 本草稿移入 `cases/defect/auto-postgres-b7848d83ed/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
