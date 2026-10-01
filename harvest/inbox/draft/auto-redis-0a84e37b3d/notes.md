# auto-redis-0a84e37b3d

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15242 (https://github.com/redis/redis/pull/15242)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 9438；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -307,7 +307,8 @@ typedef int (*RedisModuleNotificationFunc) (RedisModuleCtx *ctx, int type, const
 typedef void (*RedisModuleNotificationWithSubkeysFunc)(RedisModuleCtx *ctx, int type, const char *event, RedisModuleString *key, RedisModuleString **subkeys, int count);
 
 /* Function pointer type for post jobs */
-typedef void (*RedisModulePostNotificationJobFunc) (RedisModuleCtx *ctx, void *pd);
+typedef void (*RedisModulePostNotifyJobFunc) (RedisModuleCtx *ctx, void *pd);
+typedef void (*RedisModulePostNotifyJobPerKeyFunc) (RedisModuleCtx *ctx, RedisModuleString *key, void *pd);
 
 /* Keyspace notification subscriber information.
  * See RM_SubscribeToKeyspaceEvents() for more information. */
@@ -327,10 +328,25 @@ typedef struct RedisModuleKeyspaceSubscriber {
     int active;
 } RedisModuleKeyspaceSubscriber;
 
+/* A queued module post-notification job. A single queue holds both flavors:
+ *  - Regular jobs (RM_AddPostNotificationJob): key == NULL, `callback` is used.
+ *    They fire once at the end of the outermost execution unit and may write to
+ *    the keyspace (RM_Call).
+ *  - Per-key jobs (RM_AddPostNotificationJobForKey): key != NULL, `key_callback`
+ *    is used and receives the bound key. They may NOT write to the keyspace
+ *    (RM_Call is refused while they run), and they fire at the tail of every
+ *    call() (between MULTI/EXEC and script sub-commands) and during AOF replay,
+ *    as well as at the end of the execution unit. The key being non-NULL is what
+ *    marks a job as per-key; no separate flag is needed. */
 typedef struct RedisModulePostExecUnitJob {
     /* The module subscribed to the event */
     RedisModule *module;
-    RedisModulePostNotificationJobFunc callback;
+    union {
+        RedisModulePostNotifyJobFunc callback;           /* key == NULL */
+        RedisModulePostNotifyJobPerKeyFunc key_callback; /* key != NULL */
+    } cb;
+    RedisModuleString *key; /* NULL for a regular job; an owned reference for a
+                             * per-key job, freed after the callback runs. */
     void *pd;
     void (*free_pd)(void*);
     int dbid;
@@ -344,9 +360,14 @@ static list *moduleKeyspaceSubscribers;
 static int moduleKeyspaceSubscribersTypes = 0;
 static int moduleKeyspaceSubscribersWithSubkeysTypes = 0;
 
-/* The module post keyspace jobs list */
+/* The module post-notification jobs list. Holds both regular jobs
+ * (RM_AddPostNotificationJob) and per-key jobs (RM_AddPostNotificationJobForKey);
+ * see RedisModulePostExecUnitJob for how the two are distinguished and drained. */
 static list *modulePostExecUnitJobs;
 
+static int keyedPostNotifRMCallWarned = 0;
+static int keyedPostNotifNotifyWarned = 0;
+
 /* Data structures related to the exported dictionary data structure. */
 typedef struct RedisModuleDict {
     rax *rax;                       /* The radix tree. */
@@ -6812,7 +6833,9 @@ robj **moduleCreateArgvFromUserFormat(const char *cmdname, const char *fmt, int
  * NULL is returned and errno is set to the following values:
  *
  * * EBADF: wrong format specifier.
- * * EINVAL: wrong command arity.
+ * * EINVAL: wrong command arity, or the command was issued from within a
+ *           per-key post-notification callback (RM_AddPostNotificationJobForKey),
+ *           where commands are not allowed.
  * * ENOENT: command does not exist.
  * * EPERM: operation in Cluster instance with key in non local slot.
  * * EROFS: operation in Cluster instance when a write command is sent
@@ -6899,6 +6922,28 @@ RedisModuleCallReply *RM_Call(RedisModuleCtx *ctx, const char *cmdname, const ch
         goto cleanup;
     }
 
+    /* Enforce the per-key post-notification contract: a per-key callback
+     * (registered via RM_AddPostNotificationJobForKey) MUST NOT issue
+     * commands. */
+    if (server.firing_keyed_post_notif_jobs) {
+        /* Calling a command from within a per-key post-notification callback is
+         * a misuse of the API. */
+        if (!keyedPostNoti
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-0a84e37b3d` → 本草稿移入 `cases/defect/auto-redis-0a84e37b3d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
