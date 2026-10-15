# auto-curl-9997716b11

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #789 (https://github.com/curl/curl/pull/789)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 1674；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -544,6 +544,8 @@ struct ConnectBits {
   bool multiplex; /* connection is multiplexed */
 
   bool tcp_fastopen; /* use TCP Fast Open */
+  bool tls_enable_npn;  /* TLS NPN extension? */
+  bool tls_enable_alpn; /* TLS ALPN extension? */
 };
 
 struct hostname {
@@ -815,7 +817,7 @@ struct Curl_handler {
                                         url query strings (?foo=bar) ! */
 #define PROTOPT_CREDSPERREQUEST (1<<7) /* requires login credentials per
                                           request instead of per connection */
-
+#define PROTOPT_ALPN_NPN (1<<8) /* set ALPN and/or NPN for this */
 
 /* return the count of bytes sent, or -1 on error */
 typedef ssize_t (Curl_send)(struct connectdata *conn, /* connection data */
@@ -1671,8 +1673,8 @@ struct UserDefined {
 
   size_t maxconnects;  /* Max idle connections in the connection cache */
 
-  bool ssl_enable_npn;  /* TLS NPN extension? */
-  bool ssl_enable_alpn; /* TLS ALPN extension? */
+  bool ssl_enable_npn;      /* TLS NPN extension? */
+  bool ssl_enable_alpn;     /* TLS ALPN extension? */
   bool path_as_is;      /* allow dotdots? */
   bool pipewait;        /* wait for pipe/multiplex status before starting a
                            new connection */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-9997716b11` → 本草稿移入 `cases/defect/auto-curl-9997716b11/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
