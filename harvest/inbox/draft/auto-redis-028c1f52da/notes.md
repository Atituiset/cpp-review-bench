# auto-redis-028c1f52da

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14841 (https://github.com/redis/redis/pull/14841)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 6560；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -83,6 +83,11 @@ double R_Zero, R_PosInf, R_NegInf, R_Nan;
 /* Global vars */
 struct redisServer server; /* Server global state */
 
+/* Snapshot of server.stat_total_client_process_input_buff_events used in
+ * beforeSleep() to detect event loop cycles where client input buffers
+ * were processed. */
+long long stat_prev_total_client_process_input_buff_events = 0;
+
 /*============================ Internal prototypes ========================== */
 
 static inline int isShutdownInitiated(void);
@@ -1532,6 +1537,10 @@ int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
                                  current_time, factor);
         trackInstantaneousMetric(STATS_METRIC_EL_DURATION, server.duration_stats[EL_DURATION_TYPE_EL].sum,
                                  server.duration_stats[EL_DURATION_TYPE_EL].cnt, 1);
+
+        /* Periodic cleanup of active clients sliding window to clear stale slots
+         * when no client activity occurs for extended periods */
+        statsUpdateActiveClients(NULL);
     }
 
     /* We have just LRU_BITS bits per object for LRU information.
@@ -1994,6 +2003,17 @@ void beforeSleep(struct aeEventLoop *eventLoop) {
         }
     }
 
+    /* Detect cycles with client input processing.
+     * Compare and refresh the snapshot here (not in afterSleep()) so IO-thread updates during aeApiPoll() are not missed.
+     * Run this before dispatching new IO-thread work. */
+    if (!ProcessingEventsWhileBlocked) {
+        long long total_client_process_input_buff_events;
+        atomicGet(server.stat_total_client_process_input_buff_events, total_client_process_input_buff_events);
+        if (stat_prev_total_client_process_input_buff_events != total_client_process_input_buff_events)
+            server.stat_eventloop_cycles_with_clients_input_buff_processing++;
+        stat_prev_total_client_process_input_buff_events = total_client_process_input_buff_events;
+    }
+
     /* Handle writes with pending output buffers. */
     handleClientsWithPendingWrites();
 
@@ -2851,6 +2871,11 @@ void resetServerStats(void) {
     server.stat_cluster_incompatible_ops = 0;
     server.stat_total_prefetch_batches = 0;
     server.stat_total_prefetch_entries = 0;
+    atomicSet(server.stat_avg_pipeline_length_sum, 0);
+    atomicSet(server.stat_avg_pipeline_length_cnt, 0);
+    atomicSet(server.stat_total_client_process_input_buff_events, 0);
+    server.stat_eventloop_cycles_with_clients_input_buff_processing = 0;
+    stat_prev_total_client_process_input_buff_events = 0;
     memset(server.duration_stats, 0, sizeof(durationStats) * EL_DURATION_TYPE_NUM);
     server.el_cmd_cnt_max = 0;
     lazyfreeResetStats();
@@ -5969,6 +5994,73 @@ const char *getSafeInfoString(const char *s, size_t len, char **tmp) {
                        sizeof(unsafe_info_chars)-1);
 }
 
+/* Active Clients Sliding Window
+ *
+ * Tracks unique clients with read activity in a sliding time window.
+ * Uses a circular buffer where each slot covers SLOT_DURATION_MS milliseconds.
+ * When a client becomes active, it increments the current slot and decrements
+ * its previous slot (if it was already active within the window).
+ * The total active count is the sum of all slots in the window.
+ *
+ * Slot duration and number of slots are constant powers of 2 to enable the compiler
+ * to optimize division and modulo operations into bit shifts and bitwise ANDs. */
+
+#define WINDOW_SLOTS 4
+#define SLOT_DURATION_MS_BITS 7
+#define SLOT_DURATION_MS (1LL << SLOT_DURATION_MS_BITS) /* 128ms per slot */
+#define WINDOW_DURATION_MS (WINDOW_SLOTS * SLOT_DURATION_MS) /* 512ms total */
+
+static_assert((WINDOW_SLOTS & (WINDOW_SLOTS - 1)) == 0, "WINDOW_SLOTS must be a power of 2");
+
+static int active_clients_window[WINDOW_SLOTS];
+static long long active_clients_window_ts = 0;
+
+void statsUpdateActiveClients(client *c) {
+    mstime_t now = server.mstime;
+    int current_slot = (now / SLOT_DURATION_MS) % WINDOW_SLOTS;
+  
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-028c1f52da` → 本草稿移入 `cases/defect/auto-redis-028c1f52da/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
