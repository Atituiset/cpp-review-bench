# auto-postgres-4c1b2c7561

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
| 外部依赖数（dep_count） | 43 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #173cf85a079516fdd2b09ed853702b05fbfcd15e (https://github.com/postgres/postgres/commit/173cf85a079516fdd2b09ed853702b05fbfcd15e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 31；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 173cf85a079516fdd2b09ed853702b05fbfcd15e Fix hash index scans that start during a bucket split. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -22,14 +22,18 @@
 #include "storage/predicate.h"
 #include "utils/rel.h"
 
+static void _hash_readnext(IndexScanDesc scan, Buffer *bufp,
+						   Page *pagep, HashPageOpaque *opaquep);
+static void _hash_readprev(IndexScanDesc scan, Buffer *bufp,
+						   Page *pagep, HashPageOpaque *opaquep);
+static Buffer _hash_step_to_split_bucket(IndexScanDesc scan);
+static Buffer _hash_step_to_populated_bucket(IndexScanDesc scan);
 static bool _hash_readpage(IndexScanDesc scan, Buffer *bufP,
 						   ScanDirection dir);
 static int	_hash_load_qualified_items(IndexScanDesc scan, Page page,
 									   OffsetNumber offnum, ScanDirection dir);
 static inline void _hash_saveitem(HashScanOpaque so, int itemIndex,
 								  OffsetNumber offnum, IndexTuple itup);
-static void _hash_readnext(IndexScanDesc scan, Buffer *bufp,
-						   Page *pagep, HashPageOpaque *opaquep);
 
 /*
  *	_hash_next() -- Get the next item in a scan.
@@ -75,6 +79,13 @@ _hash_next(IndexScanDesc scan, ScanDirection dir)
 				if (!_hash_readpage(scan, &buf, dir))
 					end_of_scan = true;
 			}
+			else if (so->hashso_buc_populated && !so->hashso_buc_split)
+			{
+				buf = _hash_step_to_split_bucket(scan);
+
+				if (!_hash_readpage(scan, &buf, dir))
+					end_of_scan = true;
+			}
 			else
 				end_of_scan = true;
 		}
@@ -104,13 +115,28 @@ _hash_next(IndexScanDesc scan, ScanDirection dir)
 				if (!_hash_readpage(scan, &buf, dir))
 					end_of_scan = true;
 			}
+			else if (so->hashso_buc_populated && so->hashso_buc_split)
+			{
+				buf = _hash_step_to_populated_bucket(scan);
+
+				if (!_hash_readpage(scan, &buf, dir))
+					end_of_scan = true;
+			}
 			else
 				end_of_scan = true;
 		}
 	}
 
 	if (end_of_scan)
 	{
+		/*
+		 * A scan that started during a bucket split ends in the bucket that
+		 * its direction visits last: forward scans end in the bucket being
+		 * split, backward scans in the bucket being populated.
+		 */
+		Assert(!so->hashso_buc_populated ||
+			   so->hashso_buc_split == ScanDirectionIsForward(dir));
+
 		_hash_dropscanbuf(rel, so);
 		HashScanPosInvalidate(so->currPos);
 		return false;
@@ -162,23 +188,7 @@ _hash_readnext(IndexScanDesc scan,
 		 * end of bucket, scan bucket being split if there was a split in
 		 * progress at the start of scan.
 		 */
-		*bufp = so->hashso_split_bucket_buf;
-
-		/*
-		 * buffer for bucket being split must be valid as we acquire the pin
-		 * on it before the start of scan and retain it till end of scan.
-		 */
-		Assert(BufferIsValid(*bufp));
-
-		LockBuffer(*bufp, BUFFER_LOCK_SHARE);
-		PredicateLockPage(rel, BufferGetBlockNumber(*bufp), scan->xs_snapshot);
-
-		/*
-		 * setting hashso_buc_split to true indicates that we are scanning
-		 * bucket being split.
-		 */
-		so->hashso_buc_split = true;
-
+		*bufp = _hash_step_to_split_bucket(scan);
 		block_found = true;
 	}
 
@@ -245,28 +255,80 @@ _hash_readprev(IndexScanDesc scan,
 		 * end of bucket, scan bucket being populated if there was a split in
 		 * progress at the start of scan.
 		 */
-		*bufp = so->hashso_bucket_buf;
-
-		/*
-		 * buffer for bucket being populated must be valid as we acquire the
-		 * pin on it before the start of scan and retain it till end of scan.
-		 */
-		Assert(BufferIsValid(*bufp));
-
-		LockBuffer(*bufp, BUFFER_LOCK_SHARE);
+		*bufp = _hash_step_to_populated_bucket(scan);
 		*pagep = BufferGetPage(*bufp);
 		*opaquep = HashPageGetOpaque(*pagep);
+	}
+}
+
+/*
+ * Cross over from the bucket being populated to the bucket being split, on
+ * whose primary page we have held a pin since _hash_first.  Called on
+ * reaching the end of the populated bucket's chain while moving forward
+ * through it.
+ *
+ * Returns the split bucket's primary page, pinned and share-locked, and
+ * sets hashso_buc_split to indicate that we are now scanning that bucket.
+ */
+static Buffer
+_hash_step_to_split_bucket(IndexScanDesc scan)
+{
+	Relation	rel = scan->indexRelation;
+	HashScanOpaque so = (HashScanOpaque) scan->opaque;
+	Buffer	
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_hash_binsearch`
- 外部函数：`_hash_checkpage`
- 外部函数：`_hash_dropbuf`
- 外部函数：`_hash_dropscanbuf`
- 外部函数：`_hash_getbuf`
- 外部函数：`_hash_kill_items`
- 外部函数：`_hash_load_qualified_items`
- 外部函数：`_hash_next`
- 外部函数：`_hash_relbuf`
- 外部函数：`_hash_saveitem`
- 大写宏：`BUFFER_LOCK_SHARE`
- 大写宏：`BUFFER_LOCK_UNLOCK`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`HASH_READ`
- 大写宏：`LH_BUCKET_PAGE`
- 大写宏：`LH_OVERFLOW_PAGE`
- 外部类型：`Before`
- 外部类型：`BlockNumber`
- 外部类型：`Buffer`
- 外部类型：`Could`
- 外部类型：`Get`
- 外部类型：`HashPageOpaque`
- 外部类型：`HashScanOpaque`
- 外部类型：`If`
- 外部类型：`IndexScanDesc`
- 外部类型：`IndexTuple`
- 外部类型：`InvalidBlockNumber`
- 外部类型：`InvalidBuffer`
- 外部类型：`OffsetNumber`
- 外部类型：`Page`
- 外部类型：`Refer`
- 外部类型：`Relation`
- 外部类型：`Retain`
- 外部类型：`ScanDirection`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-4c1b2c7561` → 本草稿移入 `cases/defect/auto-postgres-4c1b2c7561/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
