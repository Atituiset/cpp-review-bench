# auto-nginx-f7dfc33997

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1167 (https://github.com/nginx/nginx/pull/1167)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -183,6 +183,17 @@ ngx_http_upstream_get_ip_hash_peer(ngx_peer_connection_t *pc, void *data)
 
     hash = iphp->hash;
 
+#if (NGX_HTTP_UPSTREAM_SID)
+    peer = ngx_http_upstream_get_rr_peer_by_sid(&iphp->rrp, pc->hint, &p, 1);
+
+    if (peer) {
+        n = p / (8 * sizeof(uintptr_t));
+        m = (uintptr_t) 1 << p % (8 * sizeof(uintptr_t));
+
+        goto found;
+    }
+#endif
+
     for ( ;; ) {
 
         for (i = 0; i < (ngx_uint_t) iphp->addrlen; i++) {
@@ -239,13 +250,21 @@ ngx_http_upstream_get_ip_hash_peer(ngx_peer_connection_t *pc, void *data)
         }
     }
 
+#if (NGX_HTTP_UPSTREAM_SID)
+found:
+#endif
+
     iphp->rrp.current = peer;
     ngx_http_upstream_rr_peer_ref(iphp->rrp.peers, peer);
 
     pc->sockaddr = peer->sockaddr;
     pc->socklen = peer->socklen;
     pc->name = &peer->name;
 
+#if (NGX_HTTP_UPSTREAM_SID)
+    pc->sid = &peer->sid;
+#endif
+
     peer->conns++;
 
     if (now - peer->checked > peer->fail_timeout) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-f7dfc33997` → 本草稿移入 `cases/defect/auto-nginx-f7dfc33997/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
