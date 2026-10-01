# auto-curl-e27187b978

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #52 (https://github.com/curl/curl/pull/52)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -421,6 +421,7 @@ static CURLcode pop3_state_user(struct connectdata *conn)
   return CURLE_OK;
 }
 
+#ifndef CURL_DISABLE_CRYPTO_AUTH
 static CURLcode pop3_state_apop(struct connectdata *conn)
 {
   CURLcode result = CURLE_OK;
@@ -454,6 +455,7 @@ static CURLcode pop3_state_apop(struct connectdata *conn)
 
   return result;
 }
+#endif
 
 static CURLcode pop3_authenticate(struct connectdata *conn)
 {
@@ -604,8 +606,10 @@ static CURLcode pop3_state_capa_resp(struct connectdata *conn,
     /* Check supported authentication types by decreasing order of security */
     if(conn->proto.pop3c.authtypes & POP3_TYPE_SASL)
       result = pop3_authenticate(conn);
+#ifndef CURL_DISABLE_CRYPTO_AUTH
     else if(conn->proto.pop3c.authtypes & POP3_TYPE_APOP)
       result = pop3_state_apop(conn);
+#endif
     else if(conn->proto.pop3c.authtypes & POP3_TYPE_CLEARTEXT)
       result = pop3_state_user(conn);
     else {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-e27187b978` → 本草稿移入 `cases/defect/auto-curl-e27187b978/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
