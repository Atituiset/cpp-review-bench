# auto-postgres-74afdd9646

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a39e53a700c5b728f2f7dab964e33b157979fb3a](https://github.com/postgres/postgres/commit/a39e53a700c5b728f2f7dab964e33b157979fb3a) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a39e53a700c5b728f2f7dab964e33b157979fb3a (https://github.com/postgres/postgres/commit/a39e53a700c5b728f2f7dab964e33b157979fb3a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a39e53a700c5b728f2f7dab964e33b157979fb3a Fix DROP DATABASE FORCE failing on parallel autovacuum workers. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -47,6 +47,7 @@
 #include "storage/bufmgr.h"
 #include "storage/proc.h"
 #include "tcop/tcopprot.h"
+#include "utils/injection_point.h"
 #include "utils/lsyscache.h"
 #include "utils/rel.h"
 
@@ -1289,6 +1290,9 @@ parallel_vacuum_main(dsm_segment *seg, shm_toc *toc)
 
 	elog(DEBUG1, "starting parallel vacuum worker");
 
+	/* Used by tests to hold a worker while it is attached to its leader */
+	INJECTION_POINT("parallel-vacuum-worker-start", NULL);
+
 	shared = (PVShared *) shm_toc_lookup(toc, PARALLEL_VACUUM_KEY_SHARED, false);
 
 	/* Set debug_query_string for individual workers */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`level`
- 外部函数：`shm_toc_lookup`
- 外部函数：`worker`
- 大写宏：`DEBUG1`
- 大写宏：`DSA`
- 大写宏：`VACUUM`
- 大写宏：`VERBOSE`
- 外部类型：`Access`
- 外部类型：`Buffer`
- 外部类型：`Cost`
- 外部类型：`Counter`
- 外部类型：`During`
- 外部类型：`Fields`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Note`
- 外部类型：`Number`
- 外部类型：`Oid`
- 外部类型：`Otherwise`
- 外部类型：`PVShared`
- 外部类型：`PVSharedCostParams`
- 外部类型：`Parallel`
- 外部类型：`Parameters`
- 外部类型：`Set`
- 外部类型：`Shared`
- 外部类型：`Statistics`
- 外部类型：`Strategy`
- 外部类型：`Target`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`TidStore`
- 外部类型：`VacDeadItemsInfo`
- 外部类型：`VacuumSharedCostBalance`
- 外部类型：`We`
- 外部类型：`slock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-74afdd9646` → 本草稿移入 `cases/defect/auto-postgres-74afdd9646/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
