# auto-postgres-97deaa6776

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#c993d9f7c8d4bcada0d5c0bd6ea32416d956fc8f](https://github.com/postgres/postgres/commit/c993d9f7c8d4bcada0d5c0bd6ea32416d956fc8f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c993d9f7c8d4bcada0d5c0bd6ea32416d956fc8f (https://github.com/postgres/postgres/commit/c993d9f7c8d4bcada0d5c0bd6ea32416d956fc8f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c993d9f7c8d4bcada0d5c0bd6ea32416d956fc8f Refuse RI fast-path row locks in read-only transactions :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -31,6 +31,7 @@
 #include "access/tableam.h"
 #include "access/xact.h"
 #include "catalog/index.h"
+#include "catalog/namespace.h"
 #include "catalog/objectaccess.h"
 #include "catalog/pg_am_d.h"
 #include "catalog/pg_collation.h"
@@ -45,6 +46,7 @@
 #include "miscadmin.h"
 #include "parser/parse_coerce.h"
 #include "parser/parse_relation.h"
+#include "tcop/utility.h"
 #include "utils/acl.h"
 #include "utils/builtins.h"
 #include "utils/datum.h"
@@ -2789,6 +2791,14 @@ ri_FastPathCheck(RI_ConstraintInfo *riinfo,
 
 	pk_rel = table_open(riinfo->pk_relid, RowShareLock);
 
+	/*
+	 * The row lock taken below is refused in a read-only transaction unless
+	 * the referenced table is temporary, as ExecCheckXactReadOnly() refuses
+	 * the SPI path's SELECT ... FOR KEY SHARE.
+	 */
+	if (XactReadOnly && !isTempNamespace(RelationGetNamespace(pk_rel)))
+		PreventCommandIfReadOnly("SELECT FOR KEY SHARE");
+
 	/*
 	 * Advance the command counter so the check sees the effects of prior
 	 * triggers in this statement, as SPI does when executing the query issued
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`table_open`
- 大写宏：`SPI`
- 外部类型：`Advance`
- 外部类型：`RowShareLock`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-97deaa6776` → 本草稿移入 `cases/defect/auto-postgres-97deaa6776/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
