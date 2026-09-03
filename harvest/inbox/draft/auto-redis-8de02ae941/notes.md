# auto-redis-8de02ae941

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15518 (https://github.com/redis/redis/pull/15518)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 51；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -48,7 +48,6 @@ void clusterSendFail(char *nodename);
 void clusterSendFailoverAuthIfNeeded(clusterNode *node, clusterMsg *request);
 void clusterUpdateState(void);
 static void clusterFireTopologyChangeEventIfNeeded(void);
-static void clusterNotifyTopologyChange(uint64_t change_flags);
 int clusterNodeCoversSlot(clusterNode *n, int slot);
 list *clusterGetNodesInMyShard(clusterNode *node);
 int clusterNodeAddSlave(clusterNode *master, clusterNode *slave);
@@ -853,7 +852,13 @@ void clusterUpdateMyselfFlags(void) {
 * The option can be set at runtime via CONFIG SET. */
 void clusterUpdateMyselfAnnouncedPorts(void) {
     if (!myself) return;
+    int old_tcp_port = myself->tcp_port;
+    int old_tls_port = myself->tls_port;
+
     deriveAnnouncedPorts(&myself->tcp_port,&myself->tls_port,&myself->cport);
+    if (myself->tcp_port != old_tcp_port || myself->tls_port != old_tls_port) {
+        clusterNotifyTopologyChange(REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_NODE);
+    }
 }
 
 /* We want to take myself->ip in sync with the cluster-announce-ip option.
@@ -881,6 +886,7 @@ void clusterUpdateMyselfIp(void) {
         } else {
             myself->ip[0] = '\0'; /* Force autodetection. */
         }
+        clusterNotifyTopologyChange(REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_NODE);
     }
 }
 
@@ -899,6 +905,7 @@ static void updateAnnouncedHostname(clusterNode *node, char *new) {
         sdsclear(node->hostname);
     }
     clusterDoBeforeSleep(CLUSTER_TODO_SAVE_CONFIG);
+    clusterNotifyTopologyChange(REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_NODE);
 }
 
 static void updateAnnouncedHumanNodename(clusterNode *node, char *new) {
@@ -2303,6 +2310,13 @@ int nodeUpdateAddressIfNeeded(clusterNode *node, clusterLink *link,
     if (node->tcp_port == tcp_port && node->cport == cport && node->tls_port == tls_port &&
         strcmp(ip,node->ip) == 0) return 0;
 
+    /* Both client ports are part of the announced node configuration and may
+     * be relevant to modules. The cluster bus port is internal, so changing it
+     * alone is not a module-visible topology change. */
+    int topology_changed = node->tcp_port != tcp_port ||
+                           node->tls_port != tls_port ||
+                           strcmp(ip,node->ip) != 0;
+
     /* IP / port is different, update it. */
     memcpy(node->ip,ip,sizeof(ip));
     node->tcp_port = tcp_port;
@@ -2318,8 +2332,8 @@ int nodeUpdateAddressIfNeeded(clusterNode *node, clusterLink *link,
     if (nodeIsSlave(myself) && myself->slaveof == node)
         replicationSetMaster(node->ip, getNodeDefaultReplicationPort(node));
 
-    /* A node moving to a different address is a topology change. */
-    clusterNotifyTopologyChange(REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_NODE);
+    if (topology_changed)
+        clusterNotifyTopologyChange(REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_NODE);
     return 1;
 }
 
@@ -5219,9 +5233,10 @@ void clusterCloseAllSlots(void) {
 
 /* Record a pending RedisModuleEvent_ClusterTopologyChange (the change_flags are
  * REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_* bits) and request it to be fired
- * from the next clusterBeforeSleep(). Called from every slot/role mutation and
- * on the cluster's OK/FAIL transition. */
-static void clusterNotifyTopologyChange(uint64_t change_flags) {
+ * from the next clusterBeforeSleep(). Called by topology mutation paths and
+ * config-driven endpoint updates. */
+void clusterNotifyTopologyChange(uint64_t change_flags) {
+    if (!server.cluster_enabled) return;
     server.cluster->topology_change_flags |= change_flags;
     clusterDoBeforeSleep(CLUSTER_TODO_FIRE_TOPOLOGY_CHANGE);
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-8de02ae941` → 本草稿移入 `cases/defect/auto-redis-8de02ae941/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
