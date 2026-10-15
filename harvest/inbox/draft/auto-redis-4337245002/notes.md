# auto-redis-4337245002

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15492 (https://github.com/redis/redis/pull/15492)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -95,6 +95,10 @@ typedef struct ConnectionType {
 
     /* Get peer username based on connection type */
     sds (*get_peer_username)(connection *conn);
+
+    /* Set the expected peer certificate name(s) to verify during the handshake
+     * (TLS only; may be NULL for connection types without certificate identity). */
+    int (*set_verify_name)(struct connection *conn, const char *name);
 } ConnectionType;
 
 struct connection {
@@ -147,6 +151,15 @@ static inline int connAccept(connection *conn, ConnectionCallbackFunc accept_han
     return conn->type->accept(conn, accept_handler);
 }
 
+/* Set the expected peer certificate name(s) (space-separated SAN/CN values) to
+ * verify during the handshake. Must be called before connConnect()/connAccept().
+ * For connection types without certificate identity (e.g. TCP/Unix) this is a
+ * no-op. Returns C_OK on success (including no-op), C_ERR if it could not be set. */
+static inline int connSetVerifyName(connection *conn, const char *name) {
+    if (conn->type->set_verify_name == NULL) return 0;
+    return conn->type->set_verify_name(conn, name);
+}
+
 /* Establish a connection.  The connect_handler will be called when the connection
  * is established, or if an error has occurred.
  *
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-4337245002` → 本草稿移入 `cases/defect/auto-redis-4337245002/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
