# auto-redis-010dbd7f3e

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #13729 (https://github.com/redis/redis/pull/13729)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -56,6 +56,7 @@ typedef struct clusterLink {
 #define CLUSTER_NODE_MEET 128     /* Send a MEET message to this node */
 #define CLUSTER_NODE_MIGRATE_TO 256 /* Master eligible for replica migration. */
 #define CLUSTER_NODE_NOFAILOVER 512 /* Slave will not try to failover. */
+#define CLUSTER_NODE_EXTENSIONS_SUPPORTED 1024 /* This node supports extensions. */
 #define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
 
 #define nodeIsMaster(n) ((n)->flags & CLUSTER_NODE_MASTER)
@@ -66,6 +67,7 @@ typedef struct clusterLink {
 #define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
 #define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)
 #define nodeCantFailover(n) ((n)->flags & CLUSTER_NODE_NOFAILOVER)
+#define nodeSupportsExtensions(n) ((n)->flags & CLUSTER_NODE_EXTENSIONS_SUPPORTED)
 
 /* Reasons why a slave is not able to failover. */
 #define CLUSTER_CANT_FAILOVER_NONE 0
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-010dbd7f3e` → 本草稿移入 `cases/defect/auto-redis-010dbd7f3e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
