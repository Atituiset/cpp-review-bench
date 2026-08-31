# auto-curl-ce0d45f71a

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #10041 (https://github.com/curl/curl/pull/10041)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1941；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1925,6 +1925,7 @@ static CURLcode imap_parse_url_options(struct connectdata *conn)
   CURLcode result = CURLE_OK;
   struct imap_conn *imapc = &conn->proto.imapc;
   const char *ptr = conn->options;
+  bool prefer_login = false;
 
   while(!result && ptr && *ptr) {
     const char *key = ptr;
@@ -1938,26 +1939,39 @@ static CURLcode imap_parse_url_options(struct connectdata *conn)
     while(*ptr && *ptr != ';')
       ptr++;
 
-    if(strncasecompare(key, "AUTH=", 5))
+    if(strncasecompare(key, "AUTH=+LOGIN", 11)) {
+      /* User prefers plaintext LOGIN over any SASL, including SASL LOGIN */
+      prefer_login = true;
+      imapc->sasl.prefmech = SASL_AUTH_NONE;
+    }
+    else if(strncasecompare(key, "AUTH=", 5)) {
+      prefer_login = false;
       result = Curl_sasl_parse_url_auth_option(&imapc->sasl,
                                                value, ptr - value);
-    else
+    }
+    else {
+      prefer_login = false;
       result = CURLE_URL_MALFORMAT;
+    }
 
     if(*ptr == ';')
       ptr++;
   }
 
-  switch(imapc->sasl.prefmech) {
-  case SASL_AUTH_NONE:
-    imapc->preftype = IMAP_TYPE_NONE;
-    break;
-  case SASL_AUTH_DEFAULT:
-    imapc->preftype = IMAP_TYPE_ANY;
-    break;
-  default:
-    imapc->preftype = IMAP_TYPE_SASL;
-    break;
+  if(prefer_login)
+    imapc->preftype = IMAP_TYPE_CLEARTEXT;
+  else {
+    switch(imapc->sasl.prefmech) {
+    case SASL_AUTH_NONE:
+      imapc->preftype = IMAP_TYPE_NONE;
+      break;
+    case SASL_AUTH_DEFAULT:
+      imapc->preftype = IMAP_TYPE_ANY;
+      break;
+    default:
+      imapc->preftype = IMAP_TYPE_SASL;
+      break;
+    }
   }
 
   return result;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-ce0d45f71a` → 本草稿移入 `cases/defect/auto-curl-ce0d45f71a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
