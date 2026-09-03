# auto-redis-eff4122db4

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15197 (https://github.com/redis/redis/pull/15197)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2838；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -47,7 +47,6 @@
 #include "cluster.h"
 #include "functions.h"
 #include "cluster_asm.h"
-#include "cluster_slot_stats.h"
 #include "bio.h"
 
 /* Operation types: import (destination side) or migrate (source side) */
@@ -2833,26 +2832,25 @@ int clusterAsmCancelBySlot(int slot, const char *reason) {
     return task ? 1 : 0;
 }
 
-/* Cancel all tasks that involve the given node. */
-int clusterAsmCancelByNode(void *node, const char *reason) {
-    if (asmManager == NULL || node == NULL) return 0;
+/* Cancel all tasks if this node is no longer a primary, and cancel tasks
+ * whose source or destination no longer exists in the current topology. */
+int clusterAsmCancelInvalidTasks(void) {
+    if (!asmManager || listLength(asmManager->tasks) == 0) return 0;
 
-    /* If the node to be deleted is myself, cancel all tasks. */
-    clusterNode *n = node;
-    if (n == getMyClusterNode()) return clusterAsmCancel(NULL, reason);
+    if (clusterNodeIsSlave(getMyClusterNode()))
+        return clusterAsmCancel(NULL, "switching to replica");
 
     int num_cancelled = 0;
     listIter li;
     listNode *ln;
     listRewind(asmManager->tasks, &li);
     while ((ln = listNext(&li)) != NULL) {
         asmTask *task = listNodeValue(ln);
-        /* Cancel the task if the source node is the one to be deleted, or
-         * the dest node is the one to be deleted. */
-        if (!memcmp(task->dest, clusterNodeGetName(n), CLUSTER_NAMELEN) ||
-            !memcmp(task->source, clusterNodeGetName(n), CLUSTER_NAMELEN))
-        {
-            asmTaskCancel(task, reason);
+        clusterNode *source = clusterLookupNode(task->source, CLUSTER_NAMELEN);
+        clusterNode *dest = clusterLookupNode(task->dest, CLUSTER_NAMELEN);
+
+        if (!source || !dest) {
+            asmTaskCancel(task, "node deleted");
             num_cancelled++;
         }
     }
@@ -2940,15 +2938,6 @@ int asmNotifyConfigUpdated(asmTask *task, sds *err) {
         return C_ERR;
     }
 
-    /* Reset per-slot statistics for the migrated/imported ranges.
-     * Note: cluster_legacy.c also cleans up, so this may run twice, but
-     * required if an alternative cluster impl is in use. */
-    for (int i = 0; i < task->slots->num_ranges; i++) {
-        slotRange *sr = &task->slots->ranges[i];
-        for (int j = sr->start; j <= sr->end; j++)
-            clusterSlotStatReset(j);
-    }
-
     /* Clear error message if successful. */
     sdsfree(task->error);
     task->error = sdsempty();
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-eff4122db4` → 本草稿移入 `cases/defect/auto-redis-eff4122db4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
