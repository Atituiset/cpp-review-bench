# auto-postgres-ae543261f3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#edbb1d2dcb18d0706e644d3048a21e4475debd95](https://github.com/postgres/postgres/commit/edbb1d2dcb18d0706e644d3048a21e4475debd95) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #edbb1d2dcb18d0706e644d3048a21e4475debd95 (https://github.com/postgres/postgres/commit/edbb1d2dcb18d0706e644d3048a21e4475debd95)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT edbb1d2dcb18d0706e644d3048a21e4475debd95 Add memory/disk usage for Function Scan nodes in EXPLAIN :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -31,6 +31,7 @@
 #include "executor/nodeBitmapIndexscan.h"
 #include "executor/nodeCustom.h"
 #include "executor/nodeForeignscan.h"
+#include "executor/nodeFunctionscan.h"
 #include "executor/nodeHash.h"
 #include "executor/nodeHashjoin.h"
 #include "executor/nodeIncrementalSort.h"
@@ -343,6 +344,10 @@ ExecParallelEstimate(PlanState *planstate, ExecParallelEstimateContext *e)
 			/* even when not parallel-aware, for EXPLAIN ANALYZE */
 			ExecMaterialEstimate((MaterialState *) planstate, e->pcxt);
 			break;
+		case T_FunctionScanState:
+			/* even when not parallel-aware, for EXPLAIN ANALYZE */
+			ExecFunctionScanEstimate((FunctionScanState *) planstate, e->pcxt);
+			break;
 		default:
 			break;
 	}
@@ -595,6 +600,11 @@ ExecParallelInitializeDSM(PlanState *planstate,
 			/* even when not parallel-aware, for EXPLAIN ANALYZE */
 			ExecMaterialInitializeDSM((MaterialState *) planstate, d->pcxt);
 			break;
+		case T_FunctionScanState:
+			/* even when not parallel-aware, for EXPLAIN ANALYZE */
+			ExecFunctionScanInitializeDSM((FunctionScanState *) planstate,
+										  d->pcxt);
+			break;
 		default:
 			break;
 	}
@@ -1084,6 +1094,7 @@ ExecParallelReInitializeDSM(PlanState *planstate,
 		case T_IncrementalSortState:
 		case T_MemoizeState:
 		case T_MaterialState:
+		case T_FunctionScanState:
 			/* these nodes have DSM state, but no reinitialization is required */
 			break;
 
@@ -1168,6 +1179,9 @@ ExecParallelRetrieveInstrumentation(PlanState *planstate,
 		case T_MaterialState:
 			ExecMaterialRetrieveInstrumentation((MaterialState *) planstate);
 			break;
+		case T_FunctionScanState:
+			ExecFunctionScanRetrieveInstrumentation((FunctionScanState *) planstate);
+			break;
 		case T_BitmapHeapScanState:
 			ExecBitmapHeapRetrieveInstrumentation((BitmapHeapScanState *) planstate);
 			break;
@@ -1501,6 +1515,11 @@ ExecParallelInitializeWorker(PlanState *planstate, ParallelWorkerContext *pwcxt)
 			/* even when not parallel-aware, for EXPLAIN ANALYZE */
 			ExecMaterialInitializeWorker((MaterialState *) planstate, pwcxt);
 			break;
+		case T_FunctionScanState:
+			/* even when not parallel-aware, for EXPLAIN ANALYZE */
+			ExecFunctionScanInitializeWorker((FunctionScanState *) planstate,
+											 pwcxt);
+			break;
 		default:
 			break;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`ANALYZE`
- 大写宏：`DSM`
- 大写宏：`EXPLAIN`
- 外部类型：`BitmapHeapScanState`
- 外部类型：`MaterialState`
- 外部类型：`T_BitmapHeapScanState`
- 外部类型：`T_IncrementalSortState`
- 外部类型：`T_MaterialState`
- 外部类型：`T_MemoizeState`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ae543261f3` → 本草稿移入 `cases/defect/auto-postgres-ae543261f3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
