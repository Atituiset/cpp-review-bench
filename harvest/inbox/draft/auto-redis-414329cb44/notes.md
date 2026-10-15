# auto-redis-414329cb44

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15556 (https://github.com/redis/redis/pull/15556)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -4,6 +4,22 @@
 #include <hiredis.h>
 #include <sdscompat.h> /* Use hiredis' sds compat header that maps sds calls to their hi_ variants */
 
+#ifdef USE_OPENSSL
+#include <openssl/ssl.h>
+#endif
+
+#if defined(TLS_NO_GROUPS)
+#define CLI_TLS_SUPPORTS_GROUPS 0
+#elif defined(SSL_CTX_set1_groups_list)
+#define CLI_TLS_SUPPORTS_GROUPS 1
+#define cliSslCtxSetGroupsList(ctx, list) SSL_CTX_set1_groups_list((ctx), (list))
+#elif defined(SSL_CTX_set1_curves_list)
+#define CLI_TLS_SUPPORTS_GROUPS 1
+#define cliSslCtxSetGroupsList(ctx, list) SSL_CTX_set1_curves_list((ctx), (list))
+#else
+#define CLI_TLS_SUPPORTS_GROUPS 0
+#endif
+
 typedef struct cliSSLconfig {
     /* Requested SNI, or NULL */
     char *sni;
@@ -21,6 +37,8 @@ typedef struct cliSSLconfig {
     char* ciphers;
     /* Preferred ciphersuites list, or NULL (applies only to TLSv1.3) */
     char* ciphersuites;
+    /* Preferred TLS named groups list, or NULL */
+    char* groups;
 } cliSSLconfig;
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-414329cb44` → 本草稿移入 `cases/defect/auto-redis-414329cb44/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
