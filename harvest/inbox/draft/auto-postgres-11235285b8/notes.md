# auto-postgres-11235285b8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b](https://github.com/postgres/postgres/commit/31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 38 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b (https://github.com/postgres/postgres/commit/31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 846；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b Add stats kind filter to pgstat_drop_matching_entries() :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -843,7 +843,8 @@ extern void pgstat_unlock_entry(PgStat_EntryRef *entry_ref);
 extern bool pgstat_drop_entry(PgStat_Kind kind, Oid dboid, uint64 objid,
 							  bool missing_ok);
 extern void pgstat_drop_all_entries(void);
-extern void pgstat_drop_matching_entries(bool (*do_drop) (PgStatShared_HashEntry *, Datum),
+extern void pgstat_drop_matching_entries(PgStat_Kind kind,
+										 bool (*do_drop) (PgStatShared_HashEntry *, Datum),
 										 Datum match_data);
 extern PgStat_EntryRef *pgstat_get_entry_ref_locked(PgStat_Kind kind, Oid dboid, uint64 objid,
 													bool nowait);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bool`
- 外部函数：`contents`
- 外部函数：`data`
- 外部函数：`dsa_get_address`
- 外部函数：`itself`
- 外部函数：`lock`
- 外部函数：`pgstat_drop_all_entries`
- 外部函数：`pgstat_drop_entry`
- 外部函数：`pgstat_drop_matching_entries`
- 外部函数：`pgstat_get_entry_ref_locked`
- 外部函数：`pgstat_reinit_entry`
- 外部函数：`retrieved`
- 外部类型：`As`
- 外部类型：`Copy`
- 外部类型：`Counter`
- 外部类型：`Datum`
- 外部类型：`Each`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`InvalidOid`
- 外部类型：`It`
- 外部类型：`LWLock`
- 外部类型：`Like`
- 外部类型：`May`
- 外部类型：`No`
- 外部类型：`Oid`
- 外部类型：`Pending`
- 外部类型：`PgStatShared_Common`
- 外部类型：`PgStatShared_HashEntry`
- 外部类型：`PgStat_EntryRef`
- 外部类型：`PgStat_HashKey`
- 外部类型：`PgStat_Kind`
- 外部类型：`PgStat_KindInfo`
- 外部类型：`Pointer`
- 外部类型：`Refcount`
- 外部类型：`Set`
- 外部类型：`The`
- 外部类型：`When`
- 外部类型：`containing`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-11235285b8` → 本草稿移入 `cases/defect/auto-postgres-11235285b8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
