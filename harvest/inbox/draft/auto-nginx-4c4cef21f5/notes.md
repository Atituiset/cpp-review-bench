# auto-nginx-4c4cef21f5

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1080 (https://github.com/nginx/nginx/pull/1080)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 176；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -19,9 +19,10 @@ typedef struct {
     ngx_queue_t                        cache;
     ngx_queue_t                        free;
 
-    ngx_http_upstream_init_pt          original_init_upstream;
     ngx_http_upstream_init_peer_pt     original_init_peer;
 
+    ngx_uint_t                         local; /* unsigned  local:1; */
+
 } ngx_http_upstream_keepalive_srv_conf_t;
 
 
@@ -34,6 +35,8 @@ typedef struct {
     socklen_t                          socklen;
     ngx_sockaddr_t                     sockaddr;
 
+    ngx_http_upstream_conf_t          *tag;
+
 } ngx_http_upstream_keepalive_cache_t;
 
 
@@ -74,14 +77,16 @@ static void ngx_http_upstream_keepalive_save_session(ngx_peer_connection_t *pc,
 #endif
 
 static void *ngx_http_upstream_keepalive_create_conf(ngx_conf_t *cf);
+static char *ngx_http_upstream_keepalive_init_main_conf(ngx_conf_t *cf,
+    void *conf);
 static char *ngx_http_upstream_keepalive(ngx_conf_t *cf, ngx_command_t *cmd,
     void *conf);
 
 
 static ngx_command_t  ngx_http_upstream_keepalive_commands[] = {
 
     { ngx_string("keepalive"),
-      NGX_HTTP_UPS_CONF|NGX_CONF_TAKE1,
+      NGX_HTTP_UPS_CONF|NGX_CONF_TAKE12,
       ngx_http_upstream_keepalive,
       NGX_HTTP_SRV_CONF_OFFSET,
       0,
@@ -117,7 +122,7 @@ static ngx_http_module_t  ngx_http_upstream_keepalive_module_ctx = {
     NULL,                                  /* postconfiguration */
 
     NULL,                                  /* create main configuration */
-    NULL,                                  /* init main configuration */
+    ngx_http_upstream_keepalive_init_main_conf, /* init main configuration */
 
     ngx_http_upstream_keepalive_create_conf, /* create server configuration */
     NULL,                                  /* merge server configuration */
@@ -143,52 +148,6 @@ ngx_module_t  ngx_http_upstream_keepalive_module = {
 };
 
 
-static ngx_int_t
-ngx_http_upstream_init_keepalive(ngx_conf_t *cf,
-    ngx_http_upstream_srv_conf_t *us)
-{
-    ngx_uint_t                               i;
-    ngx_http_upstream_keepalive_srv_conf_t  *kcf;
-    ngx_http_upstream_keepalive_cache_t     *cached;
-
-    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0,
-                   "init keepalive");
-
-    kcf = ngx_http_conf_upstream_srv_conf(us,
-                                          ngx_http_upstream_keepalive_module);
-
-    ngx_conf_init_msec_value(kcf->time, 3600000);
-    ngx_conf_init_msec_value(kcf->timeout, 60000);
-    ngx_conf_init_uint_value(kcf->requests, 1000);
-
-    if (kcf->original_init_upstream(cf, us) != NGX_OK) {
-        return NGX_ERROR;
-    }
-
-    kcf->original_init_peer = us->peer.init;
-
-    us->peer.init = ngx_http_upstream_init_keepalive_peer;
-
-    /* allocate cache items and add to free queue */
-
-    cached = ngx_pcalloc(cf->pool,
-                sizeof(ngx_http_upstream_keepalive_cache_t) * kcf->max_cached);
-    if (cached == NULL) {
-        return NGX_ERROR;
-    }
-
-    ngx_queue_init(&kcf->cache);
-    ngx_queue_init(&kcf->free);
-
-    for (i = 0; i < kcf->max_cached; i++) {
-        ngx_queue_insert_head(&kcf->free, &cached[i].queue);
-        cached[i].conf = kcf;
-    }
-
-    return NGX_OK;
-}
-
-
 static ngx_int_t
 ngx_http_upstream_init_keepalive_peer(ngx_http_request_t *r,
     ngx_http_upstream_srv_conf_t *us)
@@ -264,6 +223,10 @@ ngx_http_upstream_get_keepalive_peer(ngx_peer_connection_t *pc, void *data)
         item = ngx_queue_data(q, ngx_http_upstream_keepalive_cache_t, queue);
         c = item->connection;
 
+        if (kp->conf->local && item->tag != kp->upstream->conf) {
+            continue;
+        }
+
         if (ngx_memn2cmp((u_char *) &item->sockaddr, (u_char *) pc->sockaddr,
                          item->socklen, pc->socklen)
             == 0)
@@ -377,6 +340,7 @@ ngx_http_upstream_free_keepalive_peer(ngx_peer_connection_t *pc, void *data,
     ngx_queue_insert_head(&kp->conf->cache, q);
 
     item->connection = c;
+    item->tag = u->conf;
 
     pc->connectio
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-4c4cef21f5` → 本草稿移入 `cases/defect/auto-nginx-4c4cef21f5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
