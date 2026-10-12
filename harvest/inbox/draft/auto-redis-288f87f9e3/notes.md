# auto-redis-288f87f9e3

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15530 (https://github.com/redis/redis/pull/15530)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3175,6 +3175,9 @@ int clusterProcessPacket(clusterLink *link) {
                                     sender->shard_id,
                                     (unsigned long long)senderConfigEpoch,
                                     (unsigned long long)sender->configEpoch);
+                            /* Ignore the rest of this stale packet to prevent it from reverting
+                             * newer topology changes and creating an invalid replication chain. */
+                            return 1;
                         } else {
                             /* A failover occurred in the shard where `sender` belongs to and `sender` is no longer
                              * a primary. Update slot assignment to `master`, which is the new primary in the shard */
@@ -3237,6 +3240,33 @@ int clusterProcessPacket(clusterLink *link) {
             }
         }
 
+        /* Safeguard against sub-replicas: our master may have just been demoted
+         * above, or by an earlier packet. We cannot leave this to the same check
+         * in clusterUpdateSlotsConfigWith(), which is only reached when the
+         * sender claims slots we attribute to someone else: once the demotion
+         * handling above hands them to the new master, that difference is gone.
+         *
+         * This runs on every packet rather than only on the demotion itself, so
+         * it also recovers a node that learned of the demotion before it knew
+         * the new master. Only follow a grandmaster we believe is a master, so
+         * that a chain that has not settled yet cannot make us resync from a
+         * node that owns no slots. */
+        clusterNode *grandmaster = nodeIsSlave(myself) && myself->slaveof ?
+                                   myself->slaveof->slaveof : NULL;
+        if (grandmaster && clusterNodeIsMaster(grandmaster) && grandmaster != myself &&
+            !(server.cluster_module_flags & CLUSTER_MODULE_FLAG_NO_REDIRECTION))
+        {
+            serverLog(LL_NOTICE,
+                      "I'm a sub-replica! Reconfiguring myself as a replica of grandmaster %.40s (%s)",
+                      grandmaster->name, grandmaster->human_nodename);
+            clusterSetMaster(grandmaster);
+            /* Save the new config and broadcast to the other nodes. */
+            clusterDoBeforeSleep(CLUSTER_TODO_SAVE_CONFIG|
+                                 CLUSTER_TODO_UPDATE_STATE|
+                                 CLUSTER_TODO_FSYNC_CONFIG|
+                                 CLUSTER_TODO_BROADCAST_PONG);
+        }
+
         /* Update our info about served slots.
          *
          * Note: this MUST happen after we update the master/slave state
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-288f87f9e3` → 本草稿移入 `cases/defect/auto-redis-288f87f9e3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
