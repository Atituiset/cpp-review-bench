# auto-redis-8e26cbd029

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15371 (https://github.com/redis/redis/pull/15371)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 106；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -103,7 +103,7 @@ void linkClient(client *c) {
 static void clientSetDefaultAuth(client *c) {
     /* If the default user does not require authentication, the user is
      * directly authenticated. */
-    c->user = DefaultUser;
+    clientSetUser(c, DefaultUser);
     c->authenticated = (c->user->flags & USER_FLAG_NOPASS) &&
                        !(c->user->flags & USER_FLAG_DISABLED);
 }
@@ -193,6 +193,7 @@ client *createClient(connection *conn) {
     c->ctime = c->lastinteraction = server.unixtime;
     c->io_lastinteraction = 0;
     c->duration = 0;
+    c->user = DefaultUser; /* Set a safe default value: clientSetDefaultAuth reads c->user. */
     clientSetDefaultAuth(c);
     c->replstate = REPL_STATE_NONE;
     c->repl_start_cmd_stream_on_ack = 0;
@@ -1619,8 +1620,8 @@ void clientAcceptHandler(connection *conn) {
     if (username != NULL) {
         user *u = ACLGetUserByName(username, sdslen(username));
         if (u && !(u->flags & USER_FLAG_DISABLED)) {
-            c->user = u;
             c->authenticated = 1;
+            clientSetUser(c, u);
             moduleNotifyUserChanged(c);
             serverLog(LL_VERBOSE, "TLS: Auto-authenticated client as %s",
                       server.hide_user_data_from_log ? "*redacted*" : u->name);
@@ -2078,6 +2079,7 @@ void clearClientConnectionState(client *c) {
 }
 
 void deauthenticateAndCloseClient(client *c) {
+    disableTracking(c);
     c->user = DefaultUser;
     c->authenticated = 0;
     /* We will write replies to this client later, so we can't
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-8e26cbd029` → 本草稿移入 `cases/defect/auto-redis-8e26cbd029/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
