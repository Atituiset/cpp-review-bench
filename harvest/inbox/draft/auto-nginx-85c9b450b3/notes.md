# auto-nginx-85c9b450b3

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #208 (https://github.com/nginx/nginx/pull/208)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 166；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -163,11 +163,19 @@ ngx_http_upstream_get_ip_hash_peer(ngx_peer_connection_t *pc, void *data)
 
     ngx_http_upstream_rr_peers_rlock(iphp->rrp.peers);
 
-    if (iphp->tries > 20 || iphp->rrp.peers->single) {
+    if (iphp->tries > 20 || iphp->rrp.peers->number < 2) {
         ngx_http_upstream_rr_peers_unlock(iphp->rrp.peers);
         return iphp->get_rr_peer(pc, &iphp->rrp);
     }
 
+#if (NGX_HTTP_UPSTREAM_ZONE)
+    if (iphp->rrp.peers->config && iphp->rrp.config != *iphp->rrp.peers->config)
+    {
+        ngx_http_upstream_rr_peers_unlock(iphp->rrp.peers);
+        return iphp->get_rr_peer(pc, &iphp->rrp);
+    }
+#endif
+
     now = ngx_time();
 
     pc->cached = 0;
@@ -232,6 +240,7 @@ ngx_http_upstream_get_ip_hash_peer(ngx_peer_connection_t *pc, void *data)
     }
 
     iphp->rrp.current = peer;
+    ngx_http_upstream_rr_peer_ref(iphp->rrp.peers, peer);
 
     pc->sockaddr = peer->sockaddr;
     pc->socklen = peer->socklen;
@@ -268,6 +277,7 @@ ngx_http_upstream_ip_hash(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
     uscf->peer.init_upstream = ngx_http_upstream_init_ip_hash;
 
     uscf->flags = NGX_HTTP_UPSTREAM_CREATE
+                  |NGX_HTTP_UPSTREAM_MODIFY
                   |NGX_HTTP_UPSTREAM_WEIGHT
                   |NGX_HTTP_UPSTREAM_MAX_CONNS
                   |NGX_HTTP_UPSTREAM_MAX_FAILS
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-85c9b450b3` → 本草稿移入 `cases/defect/auto-nginx-85c9b450b3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
