# auto-redis-b97ee656f1

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #13729 (https://github.com/redis/redis/pull/13729)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 5737；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -102,6 +102,7 @@ int auxTlsPortSetter(clusterNode *n, void *value, int length);
 sds auxTlsPortGetter(clusterNode *n, sds s);
 int auxTlsPortPresent(clusterNode *n);
 static void clusterBuildMessageHdr(clusterMsg *hdr, int type, size_t msglen);
+static void updateShardId(clusterNode *node, const char *shard_id);
 
 int getNodeDefaultClientPort(clusterNode *n) {
     return server.tls_cluster ? n->tls_port : n->tcp_port;
@@ -251,12 +252,11 @@ int auxShardIdSetter(clusterNode *n, void *value, int length) {
         return C_ERR;
     }
     memcpy(n->shard_id, value, CLUSTER_NAMELEN);
-    /* if n already has replicas, make sure they all agree
-     * on the shard id */
+    /* if n already has replicas, make sure they all use
+     * the primary shard id */
     for (int i = 0; i < n->numslaves; i++) {
-        if (memcmp(n->slaves[i]->shard_id, n->shard_id, CLUSTER_NAMELEN) != 0) {
-            return C_ERR;
-        }
+        if (memcmp(n->slaves[i]->shard_id, n->shard_id, CLUSTER_NAMELEN) != 0)
+            updateShardId(n->slaves[i], n->shard_id);
     }
     clusterAddNodeToShard(value, n);
     return C_OK;
@@ -594,18 +594,12 @@ int clusterLoadConfig(char *filename) {
                 clusterAddNode(master);
             }
             /* shard_id can be absent if we are loading a nodes.conf generated
-             * by an older version of Redis; we should follow the primary's
-             * shard_id in this case */
-            if (auxFieldHandlers[af_shard_id].isPresent(n) == 0) {
-                memcpy(n->shard_id, master->shard_id, CLUSTER_NAMELEN);
-                clusterAddNodeToShard(master->shard_id, n);
-            } else if (clusterGetNodesInMyShard(master) != NULL &&
-                       memcmp(master->shard_id, n->shard_id, CLUSTER_NAMELEN) != 0)
-            {
-                /* If the primary has been added to a shard, make sure this
-                 * node has the same persisted shard id as the primary. */
-                goto fmterr;
-            }
+             * by an older version of Redis;
+             * ignore replica's shard_id in the file, only use the primary's.
+             * If replica precedes primary in file, it will be corrected
+             * later by the auxShardIdSetter */
+            memcpy(n->shard_id, master->shard_id, CLUSTER_NAMELEN);
+            clusterAddNodeToShard(master->shard_id, n);
             n->slaveof = master;
             clusterNodeAddSlave(master,n);
         } else if (auxFieldHandlers[af_shard_id].isPresent(n) == 0) {
@@ -683,6 +677,8 @@ int clusterLoadConfig(char *filename) {
     }
     /* Config sanity check */
     if (server.cluster->myself == NULL) goto fmterr;
+    if (!(myself->flags & (CLUSTER_NODE_MASTER | CLUSTER_NODE_SLAVE))) goto fmterr;
+    if (nodeIsSlave(myself) && myself->slaveof == NULL) goto fmterr;
 
     zfree(line);
     fclose(fp);
@@ -950,22 +946,47 @@ static void updateAnnouncedHumanNodename(clusterNode *node, char *new) {
     clusterDoBeforeSleep(CLUSTER_TODO_SAVE_CONFIG);
 }
 
+static void assignShardIdToNode(clusterNode *node, const char *shard_id, int flag) {
+    clusterRemoveNodeFromShard(node);
+    memcpy(node->shard_id, shard_id, CLUSTER_NAMELEN);
+    clusterAddNodeToShard(shard_id, node);
+    clusterDoBeforeSleep(flag);
+}
+
+int clusterNodeNumSlaves(clusterNode *node) {
+    return node->numslaves;
+}
+
+clusterNode *clusterNodeGetSlave(clusterNode *node, int slave_idx) {
+    return node->slaves[slave_idx];
+}
 
 static void updateShardId(clusterNode *node, const char *shard_id) {
     if (shard_id && memcmp(node->shard_id, shard_id, CLUSTER_NAMELEN) != 0) {
-        clusterRemoveNodeFromShard(node);
-        memcpy(node->shard_id, shard_id, CLUSTER_NAMELEN);
-        clusterAddNodeToShard(shard_id, node);
-        clusterDoBeforeSleep(CLUSTER_TODO_SAVE_CONFIG);
-    }
-    if (shard_id && myself != node && myself->slaveof == node) {
-        if (memcmp(myself->shard_id, shard_id, CLUSTER_NAMELEN) != 0) {
-  
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-b97ee656f1` → 本草稿移入 `cases/defect/auto-redis-b97ee656f1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
