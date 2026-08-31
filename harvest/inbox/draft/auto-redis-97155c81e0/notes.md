# auto-redis-97155c81e0

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #13637 (https://github.com/redis/redis/pull/13637)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1,3 +1,16 @@
+/*
+ * Copyright (c) 2009-Present, Redis Ltd.
+ * All rights reserved.
+ *
+ * Copyright (c) 2024-present, Valkey contributors.
+ * All rights reserved.
+ *
+ * Licensed under your choice of the Redis Source Available License 2.0
+ * (RSALv2) or the Server Side Public License v1 (SSPLv1).
+ *
+ * Portions of this file are available under BSD3 terms; see REDISCONTRIBUTIONS for more information.
+ */
+
 #ifndef CLUSTER_LEGACY_H
 #define CLUSTER_LEGACY_H
 
@@ -51,6 +64,7 @@ typedef struct clusterLink {
 #define CLUSTER_NODE_MEET 128     /* Send a MEET message to this node */
 #define CLUSTER_NODE_MIGRATE_TO 256 /* Master eligible for replica migration. */
 #define CLUSTER_NODE_NOFAILOVER 512 /* Slave will not try to failover. */
+#define CLUSTER_NODE_EXTENSIONS_SUPPORTED 1024 /* This node supports extensions. */
 #define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
 
 #define nodeIsSlave(n) ((n)->flags & CLUSTER_NODE_SLAVE)
@@ -59,6 +73,7 @@ typedef struct clusterLink {
 #define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
 #define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)
 #define nodeCantFailover(n) ((n)->flags & CLUSTER_NODE_NOFAILOVER)
+#define nodeSupportsExtensions(n) ((n)->flags & CLUSTER_NODE_EXTENSIONS_SUPPORTED)
 
 /* This structure represent elements of node->fail_reports. */
 typedef struct clusterNodeFailReport {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-97155c81e0` → 本草稿移入 `cases/defect/auto-redis-97155c81e0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
