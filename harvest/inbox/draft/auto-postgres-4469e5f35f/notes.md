# auto-postgres-4469e5f35f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#81b2179b492e3af891544813b7e22a42386b34d0](https://github.com/postgres/postgres/commit/81b2179b492e3af891544813b7e22a42386b34d0) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #81b2179b492e3af891544813b7e22a42386b34d0 (https://github.com/postgres/postgres/commit/81b2179b492e3af891544813b7e22a42386b34d0)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 978；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 81b2179b492e3af891544813b7e22a42386b34d0 Remove unused tuple expansion and copying functions :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -975,7 +975,6 @@ extern Datum heap_getsysattr(HeapTuple tup, int attnum, TupleDesc tupleDesc,
 extern Datum getmissingattr(TupleDesc tupleDesc,
 							int attnum, bool *isnull);
 extern HeapTuple heap_copytuple(HeapTuple tuple);
-extern void heap_copytuple_with_tuple(HeapTuple src, HeapTuple dest);
 extern Datum heap_copy_tuple_as_datum(HeapTuple tuple, TupleDesc tupleDesc);
 extern HeapTuple heap_form_tuple(TupleDesc tupleDescriptor,
 								 const Datum *values, const bool *isnull);
@@ -1002,8 +1001,6 @@ extern MinimalTuple heap_copy_minimal_tuple(MinimalTuple mtup, Size extra);
 extern HeapTuple heap_tuple_from_minimal_tuple(MinimalTuple mtup);
 extern MinimalTuple minimal_tuple_from_heap_tuple(HeapTuple htup, Size extra);
 extern size_t varsize_any(void *p);
-extern HeapTuple heap_expand_tuple(HeapTuple sourceTuple, TupleDesc tupleDesc);
-extern MinimalTuple minimal_expand_tuple(HeapTuple sourceTuple, TupleDesc tupleDesc);
 
 #ifndef FRONTEND
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`getmissingattr`
- 外部函数：`heap_copy_tuple_as_datum`
- 外部函数：`heap_copytuple`
- 外部函数：`heap_copytuple_with_tuple`
- 外部函数：`heap_expand_tuple`
- 外部函数：`heap_form_tuple`
- 外部函数：`heap_tuple_from_minimal_tuple`
- 外部函数：`minimal_expand_tuple`
- 外部函数：`minimal_tuple_from_heap_tuple`
- 外部函数：`varsize_any`
- 大写宏：`FRONTEND`
- 外部类型：`Datum`
- 外部类型：`HeapTuple`
- 外部类型：`MinimalTuple`
- 外部类型：`Size`
- 外部类型：`TupleDesc`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-4469e5f35f` → 本草稿移入 `cases/defect/auto-postgres-4469e5f35f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
