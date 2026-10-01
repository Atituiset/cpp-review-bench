# auto-curl-fc655614c4

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #93 (https://github.com/curl/curl/pull/93)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1329；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1323,20 +1323,26 @@ static CURLcode darwinssl_connect_step1(struct connectdata *conn,
   }
 #endif /* CURL_BUILD_MAC_10_6 || CURL_BUILD_IOS */
 
-  /* If this is a domain name and not an IP address, then configure SNI.
+  /* Configure hostname check. SNI is used if available.
+   * Both hostname check and SNI require SSLSetPeerDomainName().
    * Also: the verifyhost setting influences SNI usage */
-  /* If this is a domain name and not an IP address, then configure SNI: */
-  if((0 == Curl_inet_pton(AF_INET, conn->host.name, &addr)) &&
-#ifdef ENABLE_IPV6
-     (0 == Curl_inet_pton(AF_INET6, conn->host.name, &addr)) &&
-#endif
-     data->set.ssl.verifyhost) {
+  if(data->set.ssl.verifyhost) {
     err = SSLSetPeerDomainName(connssl->ssl_ctx, conn->host.name,
-                               strlen(conn->host.name));
+    strlen(conn->host.name));
+
     if(err != noErr) {
       infof(data, "WARNING: SSL: SSLSetPeerDomainName() failed: OSStatus %d\n",
             err);
     }
+
+    if((Curl_inet_pton(AF_INET, conn->host.name, &addr))
+  #ifdef ENABLE_IPV6
+    || (Curl_inet_pton(AF_INET6, conn->host.name, &addr))
+  #endif
+       ) {
+         infof(data, "WARNING: using IP address, SNI is being disabled by "
+         "the OS.\n");
+    }
   }
 
   /* Disable cipher suites that ST supports but are not safe. These ciphers
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-fc655614c4` → 本草稿移入 `cases/defect/auto-curl-fc655614c4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
