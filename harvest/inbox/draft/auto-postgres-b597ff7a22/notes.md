# auto-postgres-b597ff7a22

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#23192ac644e2a8f0597692aacb269321ee5acd99](https://github.com/postgres/postgres/commit/23192ac644e2a8f0597692aacb269321ee5acd99) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 45（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23192ac644e2a8f0597692aacb269321ee5acd99 (https://github.com/postgres/postgres/commit/23192ac644e2a8f0597692aacb269321ee5acd99)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 71；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 23192ac644e2a8f0597692aacb269321ee5acd99 Add pg_attribute_returns_nonnull to various memory allocation functions :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -68,32 +68,32 @@ extern PGDLLIMPORT MemoryContext CurrentMemoryContext;
 /*
  * Fundamental memory-allocation operations (more are in utils/memutils.h)
  */
-extern void *MemoryContextAlloc(MemoryContext context, Size size);
-extern void *MemoryContextAllocZero(MemoryContext context, Size size);
+pg_attribute_returns_nonnull extern void *MemoryContextAlloc(MemoryContext context, Size size);
+pg_attribute_returns_nonnull extern void *MemoryContextAllocZero(MemoryContext context, Size size);
 extern void *MemoryContextAllocExtended(MemoryContext context,
 										Size size, int flags);
 extern void *MemoryContextAllocAligned(MemoryContext context,
 									   Size size, Size alignto, int flags);
 
-extern void *palloc(Size size);
-extern void *palloc0(Size size);
+pg_attribute_returns_nonnull extern void *palloc(Size size);
+pg_attribute_returns_nonnull extern void *palloc0(Size size);
 extern void *palloc_extended(Size size, int flags);
 extern void *palloc_aligned(Size size, Size alignto, int flags);
-pg_nodiscard extern void *repalloc(void *pointer, Size size);
+pg_nodiscard pg_attribute_returns_nonnull extern void *repalloc(void *pointer, Size size);
 pg_nodiscard extern void *repalloc_extended(void *pointer,
 											Size size, int flags);
-pg_nodiscard extern void *repalloc0(void *pointer, Size oldsize, Size size);
+pg_nodiscard pg_attribute_returns_nonnull extern void *repalloc0(void *pointer, Size oldsize, Size size);
 extern void pfree(void *pointer);
 
 /*
  * Support for safe calculation of memory request sizes
  */
 extern Size add_size(Size s1, Size s2);
 extern Size mul_size(Size s1, Size s2);
-extern void *palloc_mul(Size s1, Size s2);
-extern void *palloc0_mul(Size s1, Size s2);
+pg_attribute_returns_nonnull extern void *palloc_mul(Size s1, Size s2);
+pg_attribute_returns_nonnull extern void *palloc0_mul(Size s1, Size s2);
 extern void *palloc_mul_extended(Size s1, Size s2, int flags);
-pg_nodiscard extern void *repalloc_mul(void *p, Size s1, Size s2);
+pg_nodiscard pg_attribute_returns_nonnull extern void *repalloc_mul(void *p, Size s1, Size s2);
 pg_nodiscard extern void *repalloc_mul_extended(void *p, Size s1, Size s2,
 												int flags);
 
@@ -123,8 +123,8 @@ pg_nodiscard extern void *repalloc_mul_extended(void *p, Size s1, Size s2,
 #define repalloc_array_extended(pointer, type, count, flags) ((type *) repalloc_mul_extended(pointer, sizeof(type), count, flags))
 
 /* Higher-limit allocators. */
-extern void *MemoryContextAllocHuge(MemoryContext context, Size size);
-pg_nodiscard extern void *repalloc_huge(void *pointer, Size size);
+pg_attribute_returns_nonnull extern void *MemoryContextAllocHuge(MemoryContext context, Size size);
+pg_nodiscard pg_attribute_returns_nonnull extern void *repalloc_huge(void *pointer, Size size);
 
 /*
  * Although this header file is nominally backend-only, certain frontend
@@ -154,14 +154,14 @@ extern void MemoryContextUnregisterResetCallback(MemoryContext context,
  * These are like standard strdup() except the copied string is
  * allocated in a context, not with malloc().
  */
-extern char *MemoryContextStrdup(MemoryContext context, const char *string);
-extern char *pstrdup(const char *in);
-extern char *pnstrdup(const char *in, Size len);
+pg_attribute_returns_nonnull extern char *MemoryContextStrdup(MemoryContext context, const char *string);
+pg_attribute_returns_nonnull extern char *pstrdup(const char *in);
+pg_attribute_returns_nonnull extern char *pnstrdup(const char *in, Size len);
 
-extern char *pchomp(const char *in);
+pg_attribute_returns_nonnull extern char *pchomp(const char *in);
 
 /* sprintf into a palloc'd buffer --- these are in psprintf.c */
-extern char *psprintf(const char *fmt, ...) pg_attribute_printf(1, 2);
+pg_attribute_returns_nonnull extern char *psprintf(const char *fmt, ...) pg_attribute_printf(1, 2);
 extern size_t pvsnprintf(char *buf, size_t len, const char *fmt, va_list args) pg_attribute_printf(3, 0);
 
 #endif							/* PALLO
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_size`
- 外部函数：`malloc`
- 外部函数：`mul_size`
- 外部函数：`operations`
- 外部函数：`palloc`
- 外部函数：`palloc0`
- 外部函数：`palloc0_mul`
- 外部函数：`palloc_aligned`
- 外部函数：`palloc_extended`
- 外部函数：`palloc_mul`
- 外部函数：`palloc_mul_extended`
- 外部函数：`pchomp`
- 外部函数：`pfree`
- 外部函数：`pg_attribute_printf`
- 外部函数：`pnstrdup`
- 外部函数：`psprintf`
- 外部函数：`pstrdup`
- 外部函数：`pvsnprintf`
- 外部函数：`repalloc`
- 外部函数：`repalloc0`
- 外部函数：`repalloc_array_extended`
- 外部函数：`repalloc_extended`
- 外部函数：`repalloc_huge`
- 外部函数：`repalloc_mul`
- 外部函数：`repalloc_mul_extended`
- 外部函数：`strdup`
- 大写宏：`PALLOC_H`
- 外部类型：`Although`
- 外部类型：`Fundamental`
- 外部类型：`Higher`
- 外部类型：`MemoryContextData`
- 外部类型：`Size`
- 外部类型：`Support`
- 外部类型：`These`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b597ff7a22` → 本草稿移入 `cases/defect/auto-postgres-b597ff7a22/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
