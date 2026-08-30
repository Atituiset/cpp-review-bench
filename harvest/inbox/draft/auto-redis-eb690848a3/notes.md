# auto-redis-eb690848a3

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15133 (https://github.com/redis/redis/pull/15133)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -15,12 +15,26 @@
 #ifndef MEMORY_PREFETCH_H
 #define MEMORY_PREFETCH_H
 
+#include <stddef.h>
+
 struct client;
+struct dict;
 
+/* Cross-command batch prefetching */
 void prefetchCommandsBatchInit(void);
 int determinePrefetchCount(int len);
 int addCommandToBatch(struct client *c);
 void resetCommandsBatch(void);
 void prefetchCommands(void);
 
+/* Intra-command prefetch: prefetch dict lookup data for an array of keys.
+ * Reuses the same state machine as the cross-command path. The dict's
+ * dictType drives any key/value payload prefetching via the
+ * prefetchEntryKey / prefetchEntryValue callbacks.
+ *
+ * nkeys must be <= DICT_PREFETCH_MAX_SIZE (the function asserts this).
+ * Callers should batch larger inputs into chunks of this size or smaller. */
+#define DICT_PREFETCH_MAX_SIZE 64
+void dictPrefetchKeys(struct dict **dicts, void **keys, size_t nkeys);
+
 #endif /* MEMORY_PREFETCH_H */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-eb690848a3` → 本草稿移入 `cases/defect/auto-redis-eb690848a3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
