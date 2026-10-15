# auto-curl-814f422d46

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #0dc22b690dd8dba4048d494f09a50122dd7c0dd4 (https://github.com/curl/curl/commit/0dc22b690dd8dba4048d494f09a50122dd7c0dd4)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 291；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -244,6 +244,7 @@ CURLcode Curl_ssl_session_unpack(struct Curl_easy *data,
   uint16_t val16;
   uint32_t val32;
   uint64_t val64;
+  size_t dlen;
   CURLcode result;
 
   DEBUGASSERT(buf);
@@ -271,6 +272,7 @@ CURLcode Curl_ssl_session_unpack(struct Curl_easy *data,
 
     switch(val8) {
     case CURL_SPACK_ALPN:
+      curlx_free(s->alpn);
       result = spack_decstr16(&s->alpn, &buf, end);
       if(result)
         goto out;
@@ -288,17 +290,21 @@ CURLcode Curl_ssl_session_unpack(struct Curl_easy *data,
       s->ietf_tls_id = val16;
       break;
     case CURL_SPACK_QUICTP: {
-      result = spack_decdata16(&pval8, &s->quic_tp_len, &buf, end);
+      result = spack_decdata16(&pval8, &dlen, &buf, end);
       if(result)
         goto out;
+      curlx_free(s->quic_tp);
       s->quic_tp = pval8;
+      s->quic_tp_len = dlen;
       break;
     }
     case CURL_SPACK_TICKET: {
-      result = spack_decdata16(&pval8, &s->sdata_len, &buf, end);
+      result = spack_decdata16(&pval8, &dlen, &buf, end);
       if(result)
         goto out;
+      curlx_free(s->sdata);
       s->sdata = pval8;
+      s->sdata_len = dlen;
       break;
     }
     case CURL_SPACK_VALID_UNTIL:
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-814f422d46` → 本草稿移入 `cases/defect/auto-curl-814f422d46/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
