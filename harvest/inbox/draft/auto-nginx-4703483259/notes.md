# auto-nginx-4703483259

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1022 (https://github.com/nginx/nginx/pull/1022)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -185,7 +185,13 @@ ngx_quic_cbs_release_rcd(ngx_ssl_conn_t *ssl_conn, size_t bytes_read, void *arg)
     ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0,
                    "quic ngx_quic_cbs_release_rcd len:%uz", bytes_read);
 
+    /* already closed on handshake failure */
+
     qc = ngx_quic_get_connection(c);
+    if (qc == NULL) {
+        return 1;
+    }
+
     ctx = ngx_quic_get_send_ctx(qc, qc->read_level);
 
     cl = ngx_quic_read_buffer(c, &ctx->crypto, bytes_read);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-4703483259` → 本草稿移入 `cases/defect/auto-nginx-4703483259/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
