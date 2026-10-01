# auto-nginx-74bb890ad8

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #655 (https://github.com/nginx/nginx/pull/655)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 58；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -55,7 +55,8 @@ static ssize_t ngx_quic_send_segments(ngx_connection_t *c, u_char *buf,
     size_t len, struct sockaddr *sockaddr, socklen_t socklen, size_t segment);
 #endif
 static ssize_t ngx_quic_output_packet(ngx_connection_t *c,
-    ngx_quic_send_ctx_t *ctx, u_char *data, size_t max, size_t min);
+    ngx_quic_send_ctx_t *ctx, u_char *data, size_t max, size_t min,
+    ngx_uint_t ack_only);
 static void ngx_quic_init_packet(ngx_connection_t *c, ngx_quic_send_ctx_t *ctx,
     ngx_quic_header_t *pkt, ngx_quic_path_t *path);
 static ngx_uint_t ngx_quic_get_padding_level(ngx_connection_t *c);
@@ -131,8 +132,7 @@ ngx_quic_create_datagrams(ngx_connection_t *c)
     ngx_memzero(preserved_pnum, sizeof(preserved_pnum));
 #endif
 
-    while (cg->in_flight < cg->window) {
-
+    do {
         p = dst;
 
         len = ngx_quic_path_limit(c, path, path->mtu);
@@ -158,7 +158,8 @@ ngx_quic_create_datagrams(ngx_connection_t *c)
                 return NGX_OK;
             }
 
-            n = ngx_quic_output_packet(c, ctx, p, len, min);
+            n = ngx_quic_output_packet(c, ctx, p, len, min,
+                                       cg->in_flight >= cg->window);
             if (n == NGX_ERROR) {
                 return NGX_ERROR;
             }
@@ -187,7 +188,8 @@ ngx_quic_create_datagrams(ngx_connection_t *c)
         ngx_quic_commit_send(c);
 
         path->sent += len;
-    }
+
+    } while (cg->in_flight < cg->window);
 
     return NGX_OK;
 }
@@ -315,6 +317,10 @@ ngx_quic_allow_segmentation(ngx_connection_t *c)
 
         bytes += f->len;
 
+        if (qc->congestion.in_flight + bytes >= qc->congestion.window) {
+            return 0;
+        }
+
         if (bytes > len * 3) {
             /* require at least ~3 full packets to batch */
             return 1;
@@ -364,7 +370,7 @@ ngx_quic_create_segments(ngx_connection_t *c)
 
         if (len && cg->in_flight + (p - dst) < cg->window) {
 
-            n = ngx_quic_output_packet(c, ctx, p, len, len);
+            n = ngx_quic_output_packet(c, ctx, p, len, len, 0);
             if (n == NGX_ERROR) {
                 return NGX_ERROR;
             }
@@ -521,7 +527,7 @@ ngx_quic_get_padding_level(ngx_connection_t *c)
 
 static ssize_t
 ngx_quic_output_packet(ngx_connection_t *c, ngx_quic_send_ctx_t *ctx,
-    u_char *data, size_t max, size_t min)
+    u_char *data, size_t max, size_t min, ngx_uint_t ack_only)
 {
     size_t                  len, pad, min_payload, max_payload;
     u_char                 *p;
@@ -585,6 +591,10 @@ ngx_quic_output_packet(ngx_connection_t *c, ngx_quic_send_ctx_t *ctx,
     {
         f = ngx_queue_data(q, ngx_quic_frame_t, queue);
 
+        if (ack_only && f->type != NGX_QUIC_FT_ACK) {
+            break;
+        }
+
         if (len >= max_payload) {
             break;
         }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-74bb890ad8` → 本草稿移入 `cases/defect/auto-nginx-74bb890ad8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
