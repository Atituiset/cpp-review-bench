# auto-curl-35c074346c

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #0dc22b690dd8dba4048d494f09a50122dd7c0dd4 (https://github.com/curl/curl/commit/0dc22b690dd8dba4048d494f09a50122dd7c0dd4)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 125；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -122,13 +122,13 @@ CURLcode Curl_ssl_scache_add_obj(struct Curl_cfilter *cf,
 
 /* All about an SSL session ticket */
 struct Curl_ssl_session {
-  const void *sdata;           /* session ticket data, plain bytes */
+  uint8_t *sdata;              /* session ticket data, plain bytes */
   size_t sdata_len;            /* number of bytes in sdata */
   curl_off_t valid_until;      /* seconds since EPOCH until ticket expires */
   int ietf_tls_id;             /* TLS protocol identifier negotiated */
   char *alpn;                  /* APLN TLS negotiated protocol string */
   size_t earlydata_max;        /* max 0-RTT data supported by peer */
-  const unsigned char *quic_tp; /* Optional QUIC transport param bytes */
+  uint8_t *quic_tp;            /* Optional QUIC transport param bytes */
   size_t quic_tp_len;          /* number of bytes in quic_tp */
   struct Curl_llist_node list; /*  internal storage handling */
   BIT(sectrust_verified);      /* session comes from sectrust verified TLS */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-35c074346c` → 本草稿移入 `cases/defect/auto-curl-35c074346c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
