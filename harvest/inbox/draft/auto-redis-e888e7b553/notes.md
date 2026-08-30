# auto-redis-e888e7b553

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15350 (https://github.com/redis/redis/pull/15350)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -40,6 +40,7 @@
 #define CLUSTER_TODO_FSYNC_CONFIG (1<<3)
 #define CLUSTER_TODO_HANDLE_MANUALFAILOVER (1<<4)
 #define CLUSTER_TODO_BROADCAST_PONG (1<<5)
+#define CLUSTER_TODO_FIRE_TOPOLOGY_CHANGE (1<<6) /* Fire RedisModuleEvent_ClusterTopologyChange */
 
 /* clusterLink encapsulates everything needed to talk with a remote node. */
 typedef struct clusterLink {
@@ -337,6 +338,7 @@ struct clusterState {
     uint64_t currentEpoch;
     int state;            /* CLUSTER_OK, CLUSTER_FAIL, ... */
     int size;             /* Num of master nodes with at least one slot */
+    uint64_t topology_change_flags; /* Pending RedisModuleEvent_ClusterTopologyChange reasons (REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_* bits) */
     dict *nodes;          /* Hash table of name -> clusterNode structures */
     dict *shards;         /* Hash table of shard_id -> list (of nodes) structures */
     dict *nodes_black_list; /* Nodes we don't re-add for a few seconds. */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e888e7b553` → 本草稿移入 `cases/defect/auto-redis-e888e7b553/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
