# auto-nginx-e49bf6005b

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #536 (https://github.com/nginx/nginx/pull/536)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 901；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -878,7 +878,6 @@ ngx_http_upstream_set_round_robin_peer_session(ngx_peer_connection_t *pc,
     int                            len;
     const u_char                  *p;
     ngx_http_upstream_rr_peers_t  *peers;
-    u_char                         buf[NGX_SSL_MAX_SESSION_SIZE];
 #endif
 
     peer = rrp->current;
@@ -898,12 +897,12 @@ ngx_http_upstream_set_round_robin_peer_session(ngx_peer_connection_t *pc,
 
         len = peer->ssl_session_len;
 
-        ngx_memcpy(buf, peer->ssl_session, len);
+        ngx_memcpy(ngx_ssl_session_buffer, peer->ssl_session, len);
 
         ngx_http_upstream_rr_peer_unlock(peers, peer);
         ngx_http_upstream_rr_peers_unlock(peers);
 
-        p = buf;
+        p = ngx_ssl_session_buffer;
         ssl_session = d2i_SSL_SESSION(NULL, &p, len);
 
         rc = ngx_ssl_set_session(pc->connection, ssl_session);
@@ -940,7 +939,6 @@ ngx_http_upstream_save_round_robin_peer_session(ngx_peer_connection_t *pc,
     int                            len;
     u_char                        *p;
     ngx_http_upstream_rr_peers_t  *peers;
-    u_char                         buf[NGX_SSL_MAX_SESSION_SIZE];
 #endif
 
 #if (NGX_HTTP_UPSTREAM_ZONE)
@@ -954,18 +952,18 @@ ngx_http_upstream_save_round_robin_peer_session(ngx_peer_connection_t *pc,
             return;
         }
 
-        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
-                       "save session: %p", ssl_session);
-
         len = i2d_SSL_SESSION(ssl_session, NULL);
 
+        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
+                       "save session: %p:%d", ssl_session, len);
+
         /* do not cache too big session */
 
         if (len > NGX_SSL_MAX_SESSION_SIZE) {
             return;
         }
 
-        p = buf;
+        p = ngx_ssl_session_buffer;
         (void) i2d_SSL_SESSION(ssl_session, &p);
 
         peer = rrp->current;
@@ -995,7 +993,7 @@ ngx_http_upstream_save_round_robin_peer_session(ngx_peer_connection_t *pc,
             peer->ssl_session_len = len;
         }
 
-        ngx_memcpy(peer->ssl_session, buf, len);
+        ngx_memcpy(peer->ssl_session, ngx_ssl_session_buffer, len);
 
         ngx_http_upstream_rr_peer_unlock(peers, peer);
         ngx_http_upstream_rr_peers_unlock(peers);
@@ -1023,8 +1021,6 @@ ngx_http_upstream_save_round_robin_peer_session(ngx_peer_connection_t *pc,
         ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                        "old session: %p", old_ssl_session);
 
-        /* TODO: may block */
-
         ngx_ssl_free_session(old_ssl_session);
     }
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-e49bf6005b` → 本草稿移入 `cases/defect/auto-nginx-e49bf6005b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
