# auto-redis-8ee6122987

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15468 (https://github.com/redis/redis/pull/15468)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 106；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -103,7 +103,7 @@ void linkClient(client *c) {
 static void clientSetDefaultAuth(client *c) {
     /* If the default user does not require authentication, the user is
      * directly authenticated. */
-    clientSetUser(c, DefaultUser);
+    clientSetUser(c, DefaultUser, 0);
     c->authenticated = (c->user->flags & USER_FLAG_NOPASS) &&
                        !(c->user->flags & USER_FLAG_DISABLED);
 }
@@ -1644,7 +1644,7 @@ void clientAcceptHandler(connection *conn) {
         user *u = ACLGetUserByName(username, sdslen(username));
         if (u && !(u->flags & USER_FLAG_DISABLED)) {
             c->authenticated = 1;
-            clientSetUser(c, u);
+            clientSetUser(c, u, 1);
             moduleNotifyUserChanged(c);
             serverLog(LL_VERBOSE, "TLS: Auto-authenticated client as %s",
                       server.hide_user_data_from_log ? "*redacted*" : u->name);
@@ -2054,6 +2054,24 @@ void getClientsSharedMemoryUsage(size_t *shared_mem, size_t *unshared_mem) {
     }
 }
 
+/* Drop all of the client's Pub/Sub state: unsubscribe every channel, shard
+ * channel and pattern — without notifying the client — and clear the Pub/Sub
+ * client flags (including the provenance re-auth hint). */
+static void clearClientPubSubState(client *c) {
+    /* The guard is not an optimization: this also runs on ACL-kill victims
+     * (deauthenticateAndCloseClient) that may be owned by an IO thread
+     * concurrently reading c->flags. A client with Pub/Sub state is
+     * CLIENT_PUBSUB and therefore permanently main-thread-resident (see
+     * isClientMustHandledByMainThread), so the flag writes below can never
+     * touch an IO-owned client — while running them unguarded would. */
+    if (c->flags & CLIENT_PUBSUB) {
+        pubsubUnsubscribeAllChannels(c, 0);
+        pubsubUnsubscribeShardAllChannels(c, 0);
+        pubsubUnsubscribeAllPatterns(c, 0);
+        unmarkClientAsPubSub(c);
+    }
+}
+
 /* Clear the client state to resemble a newly connected client. */
 void clearClientConnectionState(client *c) {
     listNode *ln;
@@ -2079,16 +2097,19 @@ void clearClientConnectionState(client *c) {
     c->resp = 2;
 #endif
 
+    /* Clear Pub/Sub state before resetting the ACL identity below. A still-NULL
+     * subscription is "owned by the current user", so once clientSetDefaultAuth()
+     * switches c->user to DefaultUser (it does not stamp) those entries would be
+     * momentarily attributed to DefaultUser — and moduleNotifyUserChanged() runs
+     * inside that window. Unsubscribing first removes them, so no callback ever
+     * observes a subscription under the wrong effective owner. */
+    clearClientPubSubState(c);
+
     clientSetDefaultAuth(c);
     moduleNotifyUserChanged(c);
     discardTransaction(c);
     himportFieldsetsFree(c);
 
-    pubsubUnsubscribeAllChannels(c,0);
-    pubsubUnsubscribeShardAllChannels(c, 0);
-    pubsubUnsubscribeAllPatterns(c,0);
-    unmarkClientAsPubSub(c);
-
     if (c->name) {
         decrRefCount(c->name);
         c->name = NULL;
@@ -2103,7 +2124,17 @@ void clearClientConnectionState(client *c) {
 }
 
 void deauthenticateAndCloseClient(client *c) {
+    /* The victim may be owned by an IO thread that reads c->flags concurrently:
+     * all flag writes below are guarded by flags implying main-thread residency
+     * (see clearClientPubSubState); the other writes are not read by IO threads. */
     disableTracking(c);
+    /* Clear all Pub/Sub subscriptions synchronously *before* dropping the ACL
+     * identity. This removes any provenance-stamped user* values right now, so a
+     * subsequent synchronous ACLFreeUser() (e.g. the DELUSER that triggered this
+     * kill) can never leave a dangling stamped pointer for a later ACL scan to
+     * dereference. It also prevents still-NULL subscriptions from being
+     * misattributed to DefaultUser once c->user changes below. */
+    clearClientPubSubState(c);
     c->user = DefaultUser;
     c->authenticated = 0;

```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-8ee6122987` → 本草稿移入 `cases/defect/auto-redis-8ee6122987/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
