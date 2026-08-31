# auto-redis-312ac7de7d

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15492 (https://github.com/redis/redis/pull/15492)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2791,6 +2791,41 @@ int updateClusterHumanNodename(const char **err) {
     return 1;
 }
 
+/* Validate tls-expected-peer-name at config time (startup and CONFIG SET). An
+ * empty value clears the option and is always allowed. A non-empty value must
+ * carry at least one usable name token and must not contain embedded whitespace
+ * within a name. We do not gate on the build's OpenSSL version here: the option
+ * is accepted regardless, and a build that cannot verify peer names warns once
+ * per affected connection (see tlsSetVerifyName). */
+static int isValidTlsExpectedPeerName(char *val, const char **err) {
+    if (val[0] == '\0') return 1;
+
+    /* tlsSetVerifyName() splits the value on ASCII space into individual names
+     * and passes each to X509_VERIFY_PARAM_set1_host(), which stores the string
+     * as-is without validating it. So any other whitespace (tab, newline, ...) or
+     * a whitespace-only value would be handed to OpenSSL verbatim, silently never
+     * matching any SAN and rejecting every peer while the config looks set.
+     * Require at least one name and reject any non-space whitespace; use an empty
+     * string to disable the option. */
+    int has_name = 0;
+    for (const char *p = val; *p; p++) {
+        if (*p == ' ') continue;
+        if (isspace((unsigned char)*p)) {
+            *err = "tls-expected-peer-name must not contain whitespace other than "
+                   "spaces separating names; use an empty string to disable it";
+            return 0;
+        }
+        has_name = 1;
+    }
+    if (!has_name) {
+        *err = "tls-expected-peer-name contains no usable name; "
+               "use an empty string to disable it";
+        return 0;
+    }
+
+    return 1;
+}
+
 static int applyTlsCfg(const char **err) {
     /* If TLS is enabled, try to configure OpenSSL. */
     if (server.tls_port || server.tls_replication || server.tls_cluster) {
@@ -3450,6 +3485,7 @@ standardConfig static_configs[] = {
     createStringConfig("tls-protocols", NULL, MODIFIABLE_CONFIG, EMPTY_STRING_IS_NULL, server.tls_ctx_config.protocols, NULL, NULL, applyTlsCfg),
     createStringConfig("tls-ciphers", NULL, MODIFIABLE_CONFIG, EMPTY_STRING_IS_NULL, server.tls_ctx_config.ciphers, NULL, NULL, applyTlsCfg),
     createStringConfig("tls-ciphersuites", NULL, MODIFIABLE_CONFIG, EMPTY_STRING_IS_NULL, server.tls_ctx_config.ciphersuites, NULL, NULL, applyTlsCfg),
+    createStringConfig("tls-expected-peer-name", NULL, MODIFIABLE_CONFIG, EMPTY_STRING_IS_NULL, server.tls_ctx_config.expected_peer_name, NULL, isValidTlsExpectedPeerName, NULL),
 
     /* Special configs */
     createSpecialConfig("dir", NULL, MODIFIABLE_CONFIG | PROTECTED_CONFIG | DENY_LOADING_CONFIG, setConfigDirOption, getConfigDirOption, rewriteConfigDirOption, NULL),
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-312ac7de7d` → 本草稿移入 `cases/defect/auto-redis-312ac7de7d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
