# auto-nginx-f31100ef6e

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #443 (https://github.com/nginx/nginx/pull/443)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 479；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -20,6 +20,10 @@
 /* RFC 9002, 7.6.1. Duration: kPersistentCongestionThreshold */
 #define NGX_QUIC_PERSISTENT_CONGESTION_THR   3
 
+/* CUBIC parameters x10 */
+#define NGX_QUIC_CUBIC_BETA                  7
+#define MGX_QUIC_CUBIC_C                     4
+
 
 /* send time of ACK'ed packets */
 typedef struct {
@@ -29,26 +33,30 @@ typedef struct {
 } ngx_quic_ack_stat_t;
 
 
-static ngx_inline ngx_msec_t ngx_quic_lost_threshold(ngx_quic_connection_t *qc);
+static ngx_inline ngx_msec_t ngx_quic_time_threshold(ngx_quic_connection_t *qc);
+static uint64_t ngx_quic_packet_threshold(ngx_quic_send_ctx_t *ctx);
 static void ngx_quic_rtt_sample(ngx_connection_t *c, ngx_quic_ack_frame_t *ack,
     enum ssl_encryption_level_t level, ngx_msec_t send_time);
 static ngx_int_t ngx_quic_handle_ack_frame_range(ngx_connection_t *c,
     ngx_quic_send_ctx_t *ctx, uint64_t min, uint64_t max,
     ngx_quic_ack_stat_t *st);
+static size_t ngx_quic_congestion_cubic(ngx_connection_t *c);
 static void ngx_quic_drop_ack_ranges(ngx_connection_t *c,
     ngx_quic_send_ctx_t *ctx, uint64_t pn);
 static ngx_int_t ngx_quic_detect_lost(ngx_connection_t *c,
     ngx_quic_ack_stat_t *st);
+static ngx_msec_t ngx_quic_congestion_cubic_time(ngx_connection_t *c);
 static ngx_msec_t ngx_quic_pcg_duration(ngx_connection_t *c);
 static void ngx_quic_persistent_congestion(ngx_connection_t *c);
+static ngx_msec_t ngx_quic_oldest_sent_packet(ngx_connection_t *c);
 static void ngx_quic_congestion_lost(ngx_connection_t *c,
     ngx_quic_frame_t *frame);
 static void ngx_quic_lost_handler(ngx_event_t *ev);
 
 
 /* RFC 9002, 6.1.2. Time Threshold: kTimeThreshold, kGranularity */
 static ngx_inline ngx_msec_t
-ngx_quic_lost_threshold(ngx_quic_connection_t *qc)
+ngx_quic_time_threshold(ngx_quic_connection_t *qc)
 {
     ngx_msec_t  thr;
 
@@ -59,6 +67,29 @@ ngx_quic_lost_threshold(ngx_quic_connection_t *qc)
 }
 
 
+static uint64_t
+ngx_quic_packet_threshold(ngx_quic_send_ctx_t *ctx)
+{
+    uint64_t           pkt_thr;
+    ngx_queue_t       *q;
+    ngx_quic_frame_t  *f;
+
+    if (ngx_queue_empty(&ctx->sent)) {
+        return NGX_QUIC_PKT_THR;
+    }
+
+    q = ngx_queue_head(&ctx->sent);
+    f = ngx_queue_data(q, ngx_quic_frame_t, queue);
+    pkt_thr = (ctx->pnum - f->pnum) / 2;
+
+    if (pkt_thr <= NGX_QUIC_PKT_THR) {
+        return NGX_QUIC_PKT_THR;
+    }
+
+    return pkt_thr;
+}
+
+
 ngx_int_t
 ngx_quic_handle_ack_frame(ngx_connection_t *c, ngx_quic_header_t *pkt,
     ngx_quic_frame_t *f)
@@ -313,8 +344,9 @@ ngx_quic_handle_ack_frame_range(ngx_connection_t *c, ngx_quic_send_ctx_t *ctx,
 void
 ngx_quic_congestion_ack(ngx_connection_t *c, ngx_quic_frame_t *f)
 {
+    size_t                  w_cubic;
     ngx_uint_t              blocked;
-    ngx_msec_t              timer;
+    ngx_msec_t              now, timer;
     ngx_quic_congestion_t  *cg;
     ngx_quic_connection_t  *qc;
 
@@ -329,41 +361,86 @@ ngx_quic_congestion_ack(ngx_connection_t *c, ngx_quic_frame_t *f)
         return;
     }
 
+    now = ngx_current_msec;
+
     blocked = (cg->in_flight >= cg->window) ? 1 : 0;
 
     cg->in_flight -= f->plen;
 
+    /* prevent recovery_start from wrapping */
+
+    timer = now - cg->recovery_start;
+
+    if ((ngx_msec_int_t) timer < 0) {
+        cg->recovery_start = ngx_quic_oldest_sent_packet(c) - 1;
+    }
+
     timer = f->send_time - cg->recovery_start;
 
     if ((ngx_msec_int_t) timer <= 0) {
         ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0,
-                       "quic congestion ack recovery win:%uz ss:%z if:%uz",
-                       cg->window, cg->ssthresh, cg->in_flight);
+                       "quic congestion ack rec t:%M win:%uz if:%uz",
+                       now, cg->window, cg->in_flight);
+
+        goto done;
+    }
+
+    if (cg->idle) {
+        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0,
+                       "quic congestion ack idle t:%M win:%uz if:%uz",
+                       now, cg->window, cg->in_flight);
 
         
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-f31100ef6e` → 本草稿移入 `cases/defect/auto-nginx-f31100ef6e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
