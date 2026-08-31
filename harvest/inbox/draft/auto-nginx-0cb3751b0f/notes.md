# auto-nginx-0cb3751b0f

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1633 (https://github.com/nginx/nginx/pull/1633)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 3290；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1428,37 +1428,35 @@ ngx_http_update_location_config(ngx_http_request_t *r)
 
 
 /*
- * NGX_OK       - exact or regex match
+ * NGX_OK       - exact, regex or predicate match
  * NGX_DONE     - auto redirect
  * NGX_AGAIN    - inclusive match
- * NGX_ERROR    - regex error
+ * NGX_ERROR    - regex or predicate error
  * NGX_DECLINED - no match
  */
 
 static ngx_int_t
 ngx_http_core_find_location(ngx_http_request_t *r)
 {
-    ngx_int_t                  rc;
-    ngx_http_core_loc_conf_t  *pclcf;
+    ngx_int_t                   rc;
+    ngx_uint_t                  noregex;
+    ngx_http_core_loc_conf_t   *clcf, *pclcf, **clcfp;
+    ngx_http_variable_value_t  *vv;
 #if (NGX_PCRE)
-    ngx_int_t                  n;
-    ngx_uint_t                 noregex;
-    ngx_http_core_loc_conf_t  *clcf, **clcfp;
+    ngx_int_t                   n;
+#endif
 
     noregex = 0;
-#endif
 
     pclcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
 
     rc = ngx_http_core_find_static_location(r, pclcf->static_locations);
 
     if (rc == NGX_AGAIN) {
 
-#if (NGX_PCRE)
         clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
 
         noregex = clcf->noregex;
-#endif
 
         /* look up nested locations */
 
@@ -1501,6 +1499,27 @@ ngx_http_core_find_location(ngx_http_request_t *r)
     }
 #endif
 
+    if (noregex == 0 && pclcf->predicate_locations) {
+
+        for (clcfp = pclcf->predicate_locations; *clcfp; clcfp++) {
+
+            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
+                           "test location: \"%V\"", &(*clcfp)->name);
+
+            vv = ngx_http_get_flushed_variable(r, (*clcfp)->predicate - 1);
+
+            if (vv && vv->len && (vv->len != 1 || vv->data[0] != '0')) {
+                r->loc_conf = (*clcfp)->loc_conf;
+
+                /* look up nested locations */
+
+                rc = ngx_http_core_find_location(r);
+
+                return (rc == NGX_ERROR || rc == NGX_DONE) ? rc : NGX_OK;
+            }
+        }
+    }
+
     return rc;
 }
 
@@ -3129,6 +3148,7 @@ ngx_http_core_location(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
     u_char                    *mod;
     size_t                     len;
     ngx_str_t                 *value, *name;
+    ngx_int_t                  index;
     ngx_uint_t                 i;
     ngx_conf_t                 save;
     ngx_http_module_t         *module;
@@ -3240,6 +3260,20 @@ ngx_http_core_location(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
                 }
             }
 
+        } else if (name->data[0] == '$') {
+
+            clcf->name = *name;
+
+            name->len--;
+            name->data++;
+
+            index = ngx_http_get_variable_index(cf, name);
+            if (index == NGX_ERROR) {
+                return NGX_CONF_ERROR;
+            }
+
+            clcf->predicate = index + 1;
+
         } else {
 
             clcf->name = *name;
@@ -3286,12 +3320,11 @@ ngx_http_core_location(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
 
         len = pclcf->name.len;
 
+        if (!clcf->predicate && !pclcf->predicate
 #if (NGX_PCRE)
-        if (clcf->regex == NULL
-            && ngx_filename_cmp(clcf->name.data, pclcf->name.data, len) != 0)
-#else
-        if (ngx_filename_cmp(clcf->name.data, pclcf->name.data, len) != 0)
+            && clcf->regex == NULL
 #endif
+            && ngx_filename_cmp(clcf->name.data, pclcf->name.data, len) != 0)
         {
             ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                "location \"%V\" is outside location \"%V\"",
@@ -3656,6 +3689,7 @@ ngx_http_core_create_loc_conf(ngx_conf_t *cf)
      *     clcf->error_pages = NULL;
      *     clcf->client_body_path = NULL;
      *     clcf->regex = NULL;
+     *     clcf->predicate = 0;
      *     clcf->exact_match = 0;
      *     clcf->auto_redirect = 0;
      *     clcf->alias = 0;
@@ -4674,12 +4708,16 @@ ngx_http_core_root(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
  
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-0cb3751b0f` → 本草稿移入 `cases/defect/auto-nginx-0cb3751b0f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
