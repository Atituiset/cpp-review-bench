# auto-redis-19c1ac6b18

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14848 (https://github.com/redis/redis/pull/14848)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 337；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -165,6 +165,11 @@ static void initBatchInfo(dict **dicts, GetValueDataFunc func) {
             info->state = PREFETCH_DONE;
             continue;
         }
+
+        /* We skip prefetch during loading, so ht_table[0] should never be NULL
+         * when dictSize() > 0 (which only happens mid-dictEmpty via _dictReset). */
+        serverAssert(batch->current_dicts[i]->ht_table[0]);
+
         info->ht_idx = HT_IDX_INVALID;
         info->current_entry = NULL;
         info->current_kv = NULL;
@@ -334,7 +339,7 @@ int determinePrefetchCount(int len) {
  * 3. Prefetch the keys and values for all commands in the current batch from
  *    the main dictionaries. */
 void prefetchCommands(void) {
-    if (!batch) return;
+    if (!batch || server.loading) return;
 
     /* Prefetch argv's for all pending commands */
     for (size_t i = 0; i < batch->pcmd_count; i++) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-19c1ac6b18` → 本草稿移入 `cases/defect/auto-redis-19c1ac6b18/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
