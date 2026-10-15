# auto-curl-bd3eaefdab

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #4735 (https://github.com/curl/curl/pull/4735)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 183；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -177,15 +177,6 @@ static int quic_set_encryption_secrets(SSL *ssl,
   if(level == NGTCP2_CRYPTO_LEVEL_APP) {
     if(init_ngh3_conn(qs) != CURLE_OK)
       return 0;
-
-    /* malloc an area big enough for both secrets */
-    qs->rx_secret = malloc(secretlen * 2);
-    if(!qs->rx_secret)
-      return 0;
-    memcpy(qs->rx_secret, rx_secret, secretlen);
-    memcpy(&qs->rx_secret[secretlen], tx_secret, secretlen);
-    qs->tx_secret = &qs->rx_secret[secretlen];
-    qs->rx_secretlen = secretlen;
   }
 
   return 1;
@@ -516,25 +507,6 @@ static int cb_get_new_connection_id(ngtcp2_conn *tconn, ngtcp2_cid *cid,
   return 0;
 }
 
-static int cb_update_key(ngtcp2_conn *tconn, uint8_t *rx_key,
-                         uint8_t *rx_iv, uint8_t *tx_key,
-                         uint8_t *tx_iv, void *user_data)
-{
-  struct quicsocket *qs = (struct quicsocket *)user_data;
-  uint8_t rx_secret[64];
-  uint8_t tx_secret[64];
-
-  if(ngtcp2_crypto_update_key(tconn, rx_secret, tx_secret,
-                              rx_key, rx_iv, tx_key, tx_iv, qs->rx_secret,
-                              qs->tx_secret, qs->rx_secretlen) != 0)
-    return NGTCP2_ERR_CALLBACK_FAILURE;
-
-  /* store the updated secrets */
-  memcpy(qs->rx_secret, rx_secret, qs->rx_secretlen);
-  memcpy(qs->tx_secret, tx_secret, qs->rx_secretlen);
-  return 0;
-}
-
 static ngtcp2_conn_callbacks ng_callbacks = {
   cb_initial,
   NULL, /* recv_client_initial */
@@ -556,7 +528,7 @@ static ngtcp2_conn_callbacks ng_callbacks = {
   NULL, /* rand  */
   cb_get_new_connection_id,
   NULL, /* remove_connection_id */
-  cb_update_key, /* update_key */
+  ngtcp2_crypto_update_key_cb, /* update_key */
   NULL, /* path_validation */
   NULL, /* select_preferred_addr */
   cb_stream_reset,
@@ -701,7 +673,6 @@ static CURLcode ng_disconnect(struct connectdata *conn,
   int i;
   struct quicsocket *qs = &conn->hequic[0];
   (void)dead_connection;
-  free(qs->rx_secret);
   if(qs->ssl)
     SSL_free(qs->ssl);
   for(i = 0; i < 3; i++)
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-bd3eaefdab` → 本草稿移入 `cases/defect/auto-curl-bd3eaefdab/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
