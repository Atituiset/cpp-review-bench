# auto-nginx-80accdb79e

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #562 (https://github.com/nginx/nginx/pull/562)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -749,6 +749,10 @@ ngx_http_ssl_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
     cln->data = &conf->ssl;
 
 #ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
+    {
+    static ngx_ssl_client_hello_arg cb = { ngx_http_ssl_servername };
+
+    ngx_ssl_set_client_hello_callback(conf->ssl.ctx, &cb);
 
     if (SSL_CTX_set_tlsext_servername_callback(conf->ssl.ctx,
                                                ngx_http_ssl_servername)
@@ -759,7 +763,7 @@ ngx_http_ssl_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
             "dynamically to an OpenSSL library which has no tlsext support, "
             "therefore SNI is not available");
     }
-
+    }
 #endif
 
 #ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-80accdb79e` → 本草稿移入 `cases/defect/auto-nginx-80accdb79e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
