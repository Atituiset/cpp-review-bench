# auto-redis-7334fc4981

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15065 (https://github.com/redis/redis/pull/15065)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -60,18 +60,27 @@ typedef struct vec {
     size_t cap;        /* Capacity of the vector. */
     void **data;       /* Heap-allocated storage or refers to stack. */
     void **stack;      /* Optional stack buffer. */
+    void (*free)(void *ptr); /* Optional free method, applied to each
+                              * element on vecRelease. NULL = no-op. */
 } vec;
 
 /* Return the contiguous backing array. */
-#define vecData(v) ((v)->data)
+static inline void **vecData(const vec *v) { return v->data; }
 
 /* Return the number of elements in the vector. */
-#define vecSize(v) ((v)->size)
+static inline size_t vecSize(const vec *v) { return v->size; }
 
 /* Initialize a vector */
 void vecInit(vec *v, void **stack, size_t initcap);
 
-/* Free only heap storage if any */
+/* Set a free method applied to every element on vecRelease.
+ * Symmetric to listSetFreeMethod for adlist. */
+static inline void vecSetFreeMethod(vec *v, void (*freefn)(void *ptr)) {
+    v->free = freefn;
+}
+
+/* Release storage. If a free method is set, it is applied to every element
+ * before the backing storage is released. Stack storage is never freed. */
 void vecRelease(vec *v);
 
 /* Reset the logical length to zero while preserving allocated storage. */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-7334fc4981` → 本草稿移入 `cases/defect/auto-redis-7334fc4981/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
