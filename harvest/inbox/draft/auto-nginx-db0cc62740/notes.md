# auto-nginx-db0cc62740

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #528 (https://github.com/nginx/nginx/pull/528)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 5085；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -4509,8 +4509,13 @@ ngx_http_grpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
                               prev->upstream.ssl_certificate_key, NULL);
     ngx_conf_merge_ptr_value(conf->upstream.ssl_certificate_cache,
                               prev->upstream.ssl_certificate_cache, NULL);
-    ngx_conf_merge_ptr_value(conf->upstream.ssl_passwords,
-                              prev->upstream.ssl_passwords, NULL);
+
+    if (ngx_http_upstream_merge_ssl_passwords(cf, &conf->upstream,
+                                              &prev->upstream)
+        != NGX_OK)
+    {
+        return NGX_CONF_ERROR;
+    }
 
     ngx_conf_merge_ptr_value(conf->ssl_conf_commands,
                               prev->ssl_conf_commands, NULL);
@@ -5077,16 +5082,9 @@ ngx_http_grpc_set_ssl(ngx_conf_t *cf, ngx_http_grpc_loc_conf_t *glcf)
             return NGX_ERROR;
         }
 
-        if (glcf->upstream.ssl_certificate->lengths
-            || glcf->upstream.ssl_certificate_key->lengths)
+        if (glcf->upstream.ssl_certificate->lengths == NULL
+            && glcf->upstream.ssl_certificate_key->lengths == NULL)
         {
-            glcf->upstream.ssl_passwords =
-                  ngx_ssl_preserve_passwords(cf, glcf->upstream.ssl_passwords);
-            if (glcf->upstream.ssl_passwords == NULL) {
-                return NGX_ERROR;
-            }
-
-        } else {
             if (ngx_ssl_certificate(cf, glcf->upstream.ssl,
                                     &glcf->upstream.ssl_certificate->value,
                                     &glcf->upstream.ssl_certificate_key->value,
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-db0cc62740` → 本草稿移入 `cases/defect/auto-nginx-db0cc62740/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
