# auto-nginx-7fec5e21eb

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #208 (https://github.com/nginx/nginx/pull/208)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 181；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -24,6 +24,9 @@ typedef struct {
 
 typedef struct {
     ngx_http_complex_value_t            key;
+#if (NGX_HTTP_UPSTREAM_ZONE)
+    ngx_uint_t                          config;
+#endif
     ngx_http_upstream_chash_points_t   *points;
 } ngx_http_upstream_hash_srv_conf_t;
 
@@ -49,6 +52,8 @@ static ngx_int_t ngx_http_upstream_get_hash_peer(ngx_peer_connection_t *pc,
 
 static ngx_int_t ngx_http_upstream_init_chash(ngx_conf_t *cf,
     ngx_http_upstream_srv_conf_t *us);
+static ngx_int_t ngx_http_upstream_update_chash(ngx_pool_t *pool,
+    ngx_http_upstream_srv_conf_t *us);
 static int ngx_libc_cdecl
     ngx_http_upstream_chash_cmp_points(const void *one, const void *two);
 static ngx_uint_t ngx_http_upstream_find_chash_point(
@@ -178,10 +183,17 @@ ngx_http_upstream_get_hash_peer(ngx_peer_connection_t *pc, void *data)
 
     ngx_http_upstream_rr_peers_rlock(hp->rrp.peers);
 
-    if (hp->tries > 20 || hp->rrp.peers->single || hp->key.len == 0) {
+    if (hp->tries > 20 || hp->rrp.peers->number < 2 || hp->key.len == 0) {
+        ngx_http_upstream_rr_peers_unlock(hp->rrp.peers);
+        return hp->get_rr_peer(pc, &hp->rrp);
+    }
+
+#if (NGX_HTTP_UPSTREAM_ZONE)
+    if (hp->rrp.peers->config && hp->rrp.config != *hp->rrp.peers->config) {
         ngx_http_upstream_rr_peers_unlock(hp->rrp.peers);
         return hp->get_rr_peer(pc, &hp->rrp);
     }
+#endif
 
     now = ngx_time();
 
@@ -262,6 +274,7 @@ ngx_http_upstream_get_hash_peer(ngx_peer_connection_t *pc, void *data)
     }
 
     hp->rrp.current = peer;
+    ngx_http_upstream_rr_peer_ref(hp->rrp.peers, peer);
 
     pc->sockaddr = peer->sockaddr;
     pc->socklen = peer->socklen;
@@ -284,6 +297,26 @@ ngx_http_upstream_get_hash_peer(ngx_peer_connection_t *pc, void *data)
 
 static ngx_int_t
 ngx_http_upstream_init_chash(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
+{
+    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK) {
+        return NGX_ERROR;
+    }
+
+    us->peer.init = ngx_http_upstream_init_chash_peer;
+
+#if (NGX_HTTP_UPSTREAM_ZONE)
+    if (us->shm_zone) {
+        return NGX_OK;
+    }
+#endif
+
+    return ngx_http_upstream_update_chash(cf->pool, us);
+}
+
+
+static ngx_int_t
+ngx_http_upstream_update_chash(ngx_pool_t *pool,
+    ngx_http_upstream_srv_conf_t *us)
 {
     u_char                             *host, *port, c;
     size_t                              host_len, port_len, size;
@@ -299,25 +332,32 @@ ngx_http_upstream_init_chash(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
         u_char                          byte[4];
     } prev_hash;
 
-    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK) {
-        return NGX_ERROR;
-    }
+    hcf = ngx_http_conf_upstream_srv_conf(us, ngx_http_upstream_hash_module);
 
-    us->peer.init = ngx_http_upstream_init_chash_peer;
+    if (hcf->points) {
+        ngx_free(hcf->points);
+        hcf->points = NULL;
+    }
 
     peers = us->peer.data;
     npoints = peers->total_weight * 160;
 
     size = sizeof(ngx_http_upstream_chash_points_t)
-           + sizeof(ngx_http_upstream_chash_point_t) * (npoints - 1);
+           - sizeof(ngx_http_upstream_chash_point_t)
+           + sizeof(ngx_http_upstream_chash_point_t) * npoints;
 
-    points = ngx_palloc(cf->pool, size);
+    points = pool ? ngx_palloc(pool, size) : ngx_alloc(size, ngx_cycle->log);
     if (points == NULL) {
         return NGX_ERROR;
     }
 
     points->number = 0;
 
+    if (npoints == 0) {
+        hcf->points = points;
+        return NGX_OK;
+    }
+
     for (peer = peers->peer; peer; peer = peer->next) {
         server = &peer->server;
 
@@ -401,7 +441,6 @@ ngx_http_upstream_init_chash(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
 
     points->number = i + 1;
 
-    hcf = ngx_http_conf_upstream_srv_conf(us, ngx_http_upstream_hash_module);
     hcf->points = points;
 
     return NGX_OK;
@@ -481,7 +520,22 @@ ngx_http_upstream_init_chash_peer(ngx_http_request_t *r,
 
     ngx_http_upstream_rr_peers_r
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-7fec5e21eb` → 本草稿移入 `cases/defect/auto-nginx-7fec5e21eb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
