# auto-redis-f4418a5c8f

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14750 (https://github.com/redis/redis/pull/14750)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 2153；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -24,6 +24,7 @@
 #include "cluster_slot_stats.h"
 
 #include <ctype.h>
+#include "bio.h"
 
 /* -----------------------------------------------------------------------------
  * Key space handling
@@ -1175,6 +1176,8 @@ int extractSlotFromKeysResult(robj **argv, getKeysResult *keys_result) {
  * already "down" but it is fragile to rely on the update of the global state,
  * so we also handle it here.
  *
+ * CLUSTER_REDIR_TRIMMING if the request addresses a slot that is being trimmed.
+ *
  * CLUSTER_REDIR_DOWN_STATE and CLUSTER_REDIR_DOWN_RO_STATE if the cluster is
  * down but the user attempts to execute a command that addresses one or more keys. */
 clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, int argc, int *hashslot,
@@ -1416,6 +1419,15 @@ clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, in
         return myself;
     }
 
+    /* If this node is responsible for the slot and is currently trimming it,
+     * SFLUSH may have triggered active trimming and it could still be in progress.
+     * Here we reject any write commands as no writes should be accepted for
+     * trimming slots while active trimming is in progress. */
+    if (n == myself && is_write_command && isSlotInTrimJob(slot)) {
+        if (error_code) *error_code = CLUSTER_REDIR_TRIMMING;
+        return NULL;
+    }
+
     /* Base case: just return the right node. However, if this node is not
      * myself, set error_code to MOVED since we need to issue a redirection. */
     if (n != myself && error_code) *error_code = CLUSTER_REDIR_MOVED;
@@ -1452,6 +1464,8 @@ void clusterRedirectClient(client *c, clusterNode *n, int hashslot, int error_co
                                         "-%s %d %s:%d",
                                         (error_code == CLUSTER_REDIR_ASK) ? "ASK" : "MOVED",
                                         hashslot, clusterNodePreferredEndpoint(n), port));
+    } else if (error_code == CLUSTER_REDIR_TRIMMING) {
+        addReplyError(c,"-TRYAGAIN Slot is being trimmed");
     } else {
         serverPanic("getNodeByQuery() unknown error.");
     }
@@ -1973,6 +1987,19 @@ void slotRangeArrayFreeGeneric(void *slots) {
     slotRangeArrayFree(slots);
 }
 
+/* Returns the number of keys in the given slot ranges. */
+unsigned long long getKeyCountInSlotRangeArray(slotRangeArray *slots) {
+    if (!slots) return 0;
+
+    unsigned long long key_count = 0;
+    for (int i = 0; i < slots->num_ranges; i++) {
+        for (int j = slots->ranges[i].start; j <= slots->ranges[i].end; j++) {
+            key_count += countKeysInSlot(j);
+        }
+    }
+    return key_count;
+}
+
 /* Slot range array iterator */
 slotRangeArrayIter *slotRangeArrayGetIterator(slotRangeArray *slots) {
     slotRangeArrayIter *it = zmalloc(sizeof(*it));
@@ -2100,6 +2127,7 @@ slotRangeArray *clusterGetLocalSlotRanges(void) {
  */
 void sflushCommand(client *c) {
     int flags = EMPTYDB_NO_FLAGS, argc = c->argc;
+    int trim_method = ASM_TRIM_METHOD_NONE;
 
     if (server.cluster_enabled == 0) {
         addReplyError(c,"This instance has cluster support disabled");
@@ -2127,40 +2155,73 @@ void sflushCommand(client *c) {
     slotRangeArray *slots = parseSlotRangesOrReply(c, argc, 1);
     if (!slots) return;
 
+    /* If client is AOF or master, we must obey the slot ranges.
+     * NOTE: we should exclude CLIENT_PSEUDO_MASTER when merging into fork. */
+    int must_obey = mustObeyClient(c);
+
     /* Iterate and find the slot ranges that belong to this node. Save them in
      * a new slotRangeArray. It is allocated on heap since there is a chance
      * that FLUSH SYNC will be running as blocking ASYNC and only later reply
      * with slot ranges */
-    unsigned char slots_to_flush[CLUSTER_SLOTS] = {0}; /* Requested slots to flush */
     slotRangeArray *myslots = NULL;
     for (int i = 0; i < slots->num_ranges; i++) {
         for (int j = slots->ranges[i].start; j <= slots->ranges[i].end; j++)
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-f4418a5c8f` → 本草稿移入 `cases/defect/auto-redis-f4418a5c8f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
