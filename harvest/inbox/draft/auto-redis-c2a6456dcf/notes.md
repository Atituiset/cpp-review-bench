# auto-redis-c2a6456dcf

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15182 (https://github.com/redis/redis/pull/15182)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3244,6 +3244,7 @@ standardConfig static_configs[] = {
     createIntConfig("cluster-compatibility-sample-ratio", NULL, MODIFIABLE_CONFIG, 0, 100, server.cluster_compatibility_sample_ratio, 0, INTEGER_CONFIG, NULL, NULL),
     createIntConfig("cluster-slot-migration-max-archived-tasks", NULL, MODIFIABLE_CONFIG | HIDDEN_CONFIG, 1, INT_MAX, server.asm_max_archived_tasks, 32, INTEGER_CONFIG, NULL, NULL),
     createIntConfig("lookahead", NULL, MODIFIABLE_CONFIG, 1, INT_MAX, server.lookahead, REDIS_DEFAULT_LOOKAHEAD, INTEGER_CONFIG, NULL, NULL),
+    createIntConfig("slowlog-entry-max-argc", NULL, MODIFIABLE_CONFIG, 2, INT_MAX, server.slowlog_max_argc, 32, INTEGER_CONFIG, NULL, NULL),
 
     /* Unsigned int configs */
     createUIntConfig("maxclients", NULL, MODIFIABLE_CONFIG, 1, UINT_MAX, server.maxclients, 10000, INTEGER_CONFIG, NULL, updateMaxclients),
@@ -3258,6 +3259,7 @@ standardConfig static_configs[] = {
     /* Unsigned Long configs */
     createULongConfig("active-defrag-max-scan-fields", NULL, MODIFIABLE_CONFIG, 1, LONG_MAX, server.active_defrag_max_scan_fields, 1000, INTEGER_CONFIG, NULL, NULL), /* Default: keys with more than 1000 fields will be processed separately */
     createULongConfig("slowlog-max-len", NULL, MODIFIABLE_CONFIG, 0, LONG_MAX, server.slowlog_max_len, 128, INTEGER_CONFIG, NULL, NULL),
+    createULongConfig("slowlog-entry-max-string-len", NULL, MODIFIABLE_CONFIG, 1, LONG_MAX, server.slowlog_max_string_len, 128, INTEGER_CONFIG, NULL, NULL),
     createULongConfig("acllog-max-len", NULL, MODIFIABLE_CONFIG, 0, LONG_MAX, server.acllog_max_len, 128, INTEGER_CONFIG, NULL, NULL),
 
     /* Long Long configs */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-c2a6456dcf` → 本草稿移入 `cases/defect/auto-redis-c2a6456dcf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
