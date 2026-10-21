# auto-postgres-aa12686120

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#e9d0600aba5467b352eb67aaf6ad92a9a4776b0a](https://github.com/postgres/postgres/commit/e9d0600aba5467b352eb67aaf6ad92a9a4776b0a) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e9d0600aba5467b352eb67aaf6ad92a9a4776b0a (https://github.com/postgres/postgres/commit/e9d0600aba5467b352eb67aaf6ad92a9a4776b0a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 812；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e9d0600aba5467b352eb67aaf6ad92a9a4776b0a Fix cleanup of reused pgstats entries in pgstat_release_entry_ref() :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -800,38 +800,25 @@ pgstat_release_entry_ref(PgStat_HashKey key, PgStat_EntryRef *entry_ref,
 			PgStatShared_HashEntry *shent;
 
 			/*
-			 * We're the last referrer to this entry, try to drop the shared
-			 * entry.
+			 * We're the last referrer.  Before we get the lock, the entry may
+			 * have been reinitialized, or freed and created again, so look it
+			 * up by key.  If it is gone, it got already dropped and freed.
 			 */
+			shent = dshash_find(pgStatLocal.kind_hash[key.kind], &key, true);
 
-			/* only dropped entries can reach a 0 refcount */
-			Assert(entry_ref->shared_entry->dropped);
-
-			shent = dshash_find(pgStatLocal.kind_hash[key.kind],
-								&entry_ref->shared_entry->key, true);
-			if (!shent)
-				elog(ERROR, "could not find just referenced shared stats entry");
-
-			/*
-			 * This entry may have been reinitialized while trying to release
-			 * it, so double-check that it has not been reused while holding a
-			 * lock on its shared entry.
-			 */
-			if (pg_atomic_read_u32(&entry_ref->shared_entry->generation) ==
-				entry_ref->generation)
+			if (shent)
 			{
-				/* Same "generation", so we're OK with the removal */
-				Assert(pg_atomic_read_u32(&entry_ref->shared_entry->refcount) == 0);
-				Assert(entry_ref->shared_entry == shent);
-				pgstat_free_entry(shent, NULL);
-			}
-			else
-			{
-				/*
-				 * Shared stats entry has been reinitialized, so do not drop
-				 * its shared entry, only release its lock.
-				 */
-				dshash_release_lock(pgStatLocal.kind_hash[key.kind], shent);
+				if (shent->dropped &&
+					pg_atomic_read_u32(&shent->refcount) == 0)
+				{
+					/* Entry dropped and unreferenced, so free it. */
+					pgstat_free_entry(shent, NULL);
+				}
+				else
+				{
+					/* Entry reinitialized or created again, still in use. */
+					dshash_release_lock(pgStatLocal.kind_hash[key.kind], shent);
+				}
 			}
 		}
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dsa_free`
- 外部函数：`dshash_delete_current`
- 外部函数：`dshash_delete_entry`
- 外部函数：`dshash_find`
- 外部函数：`dshash_release_lock`
- 外部函数：`elog`
- 外部函数：`pg_atomic_read_u32`
- 外部函数：`pg_atomic_sub_fetch_u64`
- 外部函数：`pgstat_get_kind_info`
- 大写宏：`ERROR`
- 大写宏：`NULL`
- 外部类型：`Decrement`
- 外部类型：`Fetch`
- 外部类型：`PgStatShared_HashEntry`
- 外部类型：`PgStat_Kind`
- 外部类型：`Same`
- 外部类型：`Shared`
- 外部类型：`This`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-aa12686120` → 本草稿移入 `cases/defect/auto-postgres-aa12686120/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
