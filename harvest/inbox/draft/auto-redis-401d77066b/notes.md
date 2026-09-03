# auto-redis-401d77066b

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15338 (https://github.com/redis/redis/pull/15338)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 878；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -66,8 +66,11 @@
 #define CLUSTER_MANAGER_SLOTS               16384
 #define CLUSTER_MANAGER_PORT_INCR           10000 /* same as CLUSTER_PORT_INCR */
 #define CLUSTER_MANAGER_MIGRATE_TIMEOUT     60000
+#define CLUSTER_MANAGER_ASM_MIGRATE_TIMEOUT 3600000 /* 60 minutes */
 #define CLUSTER_MANAGER_MIGRATE_PIPELINE    10
 #define CLUSTER_MANAGER_REBALANCE_THRESHOLD 2
+/* CLUSTER MIGRATION, used by ASM, is available starting with Redis 8.4.0. */
+#define CLUSTER_MANAGER_ASM_MIN_VERSION     "8.4.0"
 
 #define CLUSTER_MANAGER_INVALID_HOST_ARG \
     "[ERR] Invalid arguments: you need to pass either a valid " \
@@ -116,6 +119,7 @@
 #define CLUSTER_MANAGER_CMD_FLAG_FIX_WITH_UNREACHABLE_MASTERS 1 << 10
 #define CLUSTER_MANAGER_CMD_FLAG_MASTERS_ONLY   1 << 11
 #define CLUSTER_MANAGER_CMD_FLAG_SLAVES_ONLY    1 << 12
+#define CLUSTER_MANAGER_CMD_FLAG_TIMEOUT        1 << 13
 
 #define CLUSTER_MANAGER_OPT_GETFRIENDS  1 << 0
 #define CLUSTER_MANAGER_OPT_COLD        1 << 1
@@ -867,20 +871,16 @@ static size_t cliLegacyCountCommands(struct commandDocs *commands, sds version)
     return numCommands;
 }
 
-/* Gets the server version string by calling INFO SERVER.
- * Stores the result in config.server_version.
- * When not connected, or not possible, returns NULL. */
-static sds cliGetServerVersion(void) {
+/* Gets the server version string from the given context by calling INFO SERVER.
+ * The caller owns the returned SDS. When not connected, or not possible,
+ * returns NULL. */
+static sds cliGetServerVersionFromContext(redisContext *ctx) {
     static const char *key = "\nredis_version:";
     redisReply *serverInfo = NULL;
     char *pos;
 
-    if (config.server_version != NULL) {
-        return config.server_version;
-    }
-
-    if (!context) return NULL;
-    serverInfo = redisCommand(context, "INFO SERVER");
+    if (!ctx) return NULL;
+    serverInfo = redisCommand(ctx, "INFO SERVER");
     if (serverInfo == NULL || serverInfo->type == REDIS_REPLY_ERROR) {
         freeReplyObject(serverInfo);
         return sdsempty();
@@ -897,14 +897,28 @@ static sds cliGetServerVersion(void) {
         if (end) {
             sds version = sdsnewlen(pos, end - pos);
             freeReplyObject(serverInfo);
-            config.server_version = version;
             return version;
         }
     }
     freeReplyObject(serverInfo);
     return NULL;
 }
 
+/* Gets the server version string by calling INFO SERVER.
+ * Stores the result in config.server_version.
+ * When not connected, or not possible, returns NULL. */
+static sds cliGetServerVersion(void) {
+    if (config.server_version != NULL) {
+        return config.server_version;
+    }
+
+    sds version = cliGetServerVersionFromContext(context);
+    if (version != NULL && sdslen(version) != 0) {
+        config.server_version = version;
+    }
+    return version;
+}
+
 static void cliLegacyInitHelp(dict *groups) {
     sds serverVersion = cliGetServerVersion();
     
@@ -2970,6 +2984,8 @@ static int parseOptions(int argc, char **argv) {
             config.cluster_manager_command.slots = atoi(argv[++i]);
         } else if (!strcmp(argv[i],"--cluster-timeout") && !lastarg) {
             config.cluster_manager_command.timeout = atoi(argv[++i]);
+            config.cluster_manager_command.flags |=
+                CLUSTER_MANAGER_CMD_FLAG_TIMEOUT;
         } else if (!strcmp(argv[i],"--cluster-pipeline") && !lastarg) {
             config.cluster_manager_command.pipeline = atoi(argv[++i]);
         } else if (!strcmp(argv[i],"--cluster-threshold") && !lastarg) {
@@ -3826,6 +3842,7 @@ typedef struct clusterManagerNode {
     int importing_count; /* Length of the importing array (importing slots*2) */
     float weight;   /* Weight used by rebalance */
     int balance;    /* Used by rebalance */
+    sds server_version;
 } clusterManagerNode;
 
 /* Data structure used to represent a sequence of cluster nodes. */
@@ -3943,11 +3960,11 @@ clusterManagerCommandDef clusterManagerCommands[] =
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-401d77066b` → 本草稿移入 `cases/defect/auto-redis-401d77066b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
