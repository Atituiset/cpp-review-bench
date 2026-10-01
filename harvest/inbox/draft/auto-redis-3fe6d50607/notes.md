# auto-redis-3fe6d50607

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15133 (https://github.com/redis/redis/pull/15133)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -135,6 +135,25 @@ typedef struct dictType {
 
     /* Optional callback called when the dict is destroyed. */
     void (*onDictRelease)(dict *d);
+
+    /* Optional prefetch hooks used by the memory_prefetch state machine.
+     * Both default to NULL; when both are NULL the state machine just
+     * prefetches the bucket + dictEntry chain and stops there.
+     *
+     * prefetchEntryKey: called after a dictEntry has been brought into
+     *   cache. Returns an address to issue redis_prefetch_read on (so the
+     *   key payload behind the entry is warm before keyCompare runs), or
+     *   NULL if nothing extra is needed (e.g. the key is co-located with
+     *   the entry).
+     * prefetchEntryValue: called when the entry is the *presumed* match
+     *   for the lookup key — either keyCompare returned equal, or the
+     *   state machine took the "last entry in chain, not rehashing"
+     *   shortcut and is betting on a hit without comparing. Callbacks
+     *   must therefore not assume the key has been verified equal; the
+     *   prefetch is advisory. Returns an address to prefetch for the
+     *   value-side payload, or NULL. */
+    void *(*prefetchEntryKey)(const dictEntry *de);
+    void *(*prefetchEntryValue)(const dictEntry *de);
 } dictType;
 
 #define DICTHT_SIZE(exp) ((exp) == -1 ? 0 : (unsigned long)1<<(exp))
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-3fe6d50607` → 本草稿移入 `cases/defect/auto-redis-3fe6d50607/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
