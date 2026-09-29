# auto-curl-40276c3c8b

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #8506 (https://github.com/curl/curl/pull/8506)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1040；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1037,7 +1037,7 @@ static CURLcode smtp_state_command_resp(struct Curl_easy *data, int smtpcode,
   if((smtp->rcpt && smtpcode/100 != 2 && smtpcode != 553 && smtpcode != 1) ||
      (!smtp->rcpt && smtpcode/100 != 2 && smtpcode != 1)) {
     failf(data, "Command failed: %d", smtpcode);
-    result = CURLE_RECV_ERROR;
+    result = CURLE_WEIRD_SERVER_REPLY;
   }
   else {
     /* Temporarily add the LF character back and send as body to the client */
@@ -1182,7 +1182,7 @@ static CURLcode smtp_state_postdata_resp(struct Curl_easy *data,
   (void)instate; /* no use for this yet */
 
   if(smtpcode != 250)
-    result = CURLE_RECV_ERROR;
+    result = CURLE_WEIRD_SERVER_REPLY;
 
   /* End of DONE phase */
   state(data, SMTP_STOP);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-40276c3c8b` → 本草稿移入 `cases/defect/auto-curl-40276c3c8b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
