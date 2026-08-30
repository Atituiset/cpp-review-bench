# auto-nginx-a968dc4006

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1441 (https://github.com/nginx/nginx/pull/1441)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2192；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2181,6 +2181,8 @@ ngx_http_v2_state_settings(ngx_http_v2_connection_t *h2c, u_char *pos,
         return ngx_http_v2_connection_error(h2c, NGX_HTTP_V2_SIZE_ERROR);
     }
 
+    h2c->state.window_delta = 0;
+
     return ngx_http_v2_state_settings_params(h2c, pos, end);
 }
 
@@ -2189,12 +2191,9 @@ static u_char *
 ngx_http_v2_state_settings_params(ngx_http_v2_connection_t *h2c, u_char *pos,
     u_char *end)
 {
-    ssize_t                   window_delta;
     ngx_uint_t                id, value;
     ngx_http_v2_out_frame_t  *frame;
 
-    window_delta = 0;
-
     while (h2c->state.length) {
         if (end - pos < NGX_HTTP_V2_SETTINGS_PARAM_SIZE) {
             return ngx_http_v2_state_save(h2c, pos, end,
@@ -2222,7 +2221,8 @@ ngx_http_v2_state_settings_params(ngx_http_v2_connection_t *h2c, u_char *pos,
                                                   NGX_HTTP_V2_FLOW_CTRL_ERROR);
             }
 
-            window_delta = value - h2c->init_window;
+            h2c->state.window_delta = (ssize_t) value
+                                      - (ssize_t) h2c->init_window;
             break;
 
         case NGX_HTTP_V2_MAX_FRAME_SIZE_SETTING:
@@ -2275,13 +2275,16 @@ ngx_http_v2_state_settings_params(ngx_http_v2_connection_t *h2c, u_char *pos,
 
     ngx_http_v2_queue_ordered_frame(h2c, frame);
 
-    if (window_delta) {
-        h2c->init_window += window_delta;
+    if (h2c->state.window_delta) {
+        h2c->init_window += h2c->state.window_delta;
 
-        if (ngx_http_v2_adjust_windows(h2c, window_delta) != NGX_OK) {
+        if (ngx_http_v2_adjust_windows(h2c, h2c->state.window_delta) != NGX_OK)
+        {
             return ngx_http_v2_connection_error(h2c,
                                                 NGX_HTTP_V2_INTERNAL_ERROR);
         }
+
+        h2c->state.window_delta = 0;
     }
 
     return ngx_http_v2_state_complete(h2c, pos, end);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-a968dc4006` → 本草稿移入 `cases/defect/auto-nginx-a968dc4006/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
