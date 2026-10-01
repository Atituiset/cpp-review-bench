# auto-nginx-9343ef7230

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #443 (https://github.com/nginx/nginx/pull/443)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 311；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -308,11 +308,16 @@ ngx_quic_new_connection(ngx_connection_t *c, ngx_quic_conf_t *conf,
     qc->streams.client_max_streams_uni = qc->tp.initial_max_streams_uni;
     qc->streams.client_max_streams_bidi = qc->tp.initial_max_streams_bidi;
 
-    qc->congestion.window = ngx_min(10 * qc->tp.max_udp_payload_size,
-                                    ngx_max(2 * qc->tp.max_udp_payload_size,
+    qc->congestion.window = ngx_min(10 * NGX_QUIC_MIN_INITIAL_SIZE,
+                                    ngx_max(2 * NGX_QUIC_MIN_INITIAL_SIZE,
                                             14720));
     qc->congestion.ssthresh = (size_t) -1;
-    qc->congestion.recovery_start = ngx_current_msec;
+    qc->congestion.mtu = NGX_QUIC_MIN_INITIAL_SIZE;
+    qc->congestion.recovery_start = ngx_current_msec - 1;
+
+    qc->max_frames = (conf->max_concurrent_streams_uni
+                      + conf->max_concurrent_streams_bidi)
+                     * conf->stream_buffer_size / 2000;
 
     if (pkt->validated && pkt->retried) {
         qc->tp.retry_scid.len = pkt->dcid.len;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-9343ef7230` → 本草稿移入 `cases/defect/auto-nginx-9343ef7230/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
