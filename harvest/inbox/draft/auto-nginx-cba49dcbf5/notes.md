# auto-nginx-cba49dcbf5

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1147 (https://github.com/nginx/nginx/pull/1147)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 321；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -318,8 +318,8 @@ static ngx_int_t ngx_http_mp4_update_stts_atom(ngx_http_mp4_file_t *mp4,
     ngx_http_mp4_trak_t *trak);
 static ngx_int_t ngx_http_mp4_crop_stts_data(ngx_http_mp4_file_t *mp4,
     ngx_http_mp4_trak_t *trak, ngx_uint_t start);
-static uint32_t ngx_http_mp4_seek_key_frame(ngx_http_mp4_file_t *mp4,
-    ngx_http_mp4_trak_t *trak, uint32_t start_sample);
+static ngx_int_t ngx_http_mp4_seek_key_frame(ngx_http_mp4_file_t *mp4,
+    ngx_http_mp4_trak_t *trak, uint32_t start_sample, uint32_t *key_prefix);
 static ngx_int_t ngx_http_mp4_read_stss_atom(ngx_http_mp4_file_t *mp4,
     uint64_t atom_data_size);
 static ngx_int_t ngx_http_mp4_update_stss_atom(ngx_http_mp4_file_t *mp4,
@@ -2455,7 +2455,11 @@ ngx_http_mp4_crop_stts_data(ngx_http_mp4_file_t *mp4,
 found:
 
     if (start) {
-        key_prefix = ngx_http_mp4_seek_key_frame(mp4, trak, start_sample);
+        if (ngx_http_mp4_seek_key_frame(mp4, trak, start_sample, &key_prefix)
+            != NGX_OK)
+        {
+            return NGX_ERROR;
+        }
 
         start_sample -= key_prefix;
 
@@ -2499,22 +2503,24 @@ ngx_http_mp4_crop_stts_data(ngx_http_mp4_file_t *mp4,
 }
 
 
-static uint32_t
+static ngx_int_t
 ngx_http_mp4_seek_key_frame(ngx_http_mp4_file_t *mp4, ngx_http_mp4_trak_t *trak,
-    uint32_t start_sample)
+    uint32_t start_sample, uint32_t *key_prefix)
 {
-    uint32_t              key_prefix, sample, *entry, *end;
+    uint32_t              sample, *entry, *end;
     ngx_buf_t            *data;
     ngx_http_mp4_conf_t  *conf;
 
+    *key_prefix = 0;
+
     conf = ngx_http_get_module_loc_conf(mp4->request, ngx_http_mp4_module);
     if (!conf->start_key_frame) {
-        return 0;
+        return NGX_OK;
     }
 
     data = trak->out[NGX_HTTP_MP4_STSS_DATA].buf;
     if (data == NULL) {
-        return 0;
+        return NGX_OK;
     }
 
     entry = (uint32_t *) data->pos;
@@ -2523,22 +2529,28 @@ ngx_http_mp4_seek_key_frame(ngx_http_mp4_file_t *mp4, ngx_http_mp4_trak_t *trak,
     /* sync samples starts from 1 */
     start_sample++;
 
-    key_prefix = 0;
-
     while (entry < end) {
         sample = ngx_mp4_get_32value(entry);
+
+        if (sample == 0) {
+            ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
+                          "zero sync sample in \"%s\"",
+                          mp4->file.name.data);
+            return NGX_ERROR;
+        }
+
         if (sample > start_sample) {
             break;
         }
 
-        key_prefix = start_sample - sample;
+        *key_prefix = start_sample - sample;
         entry++;
     }
 
     ngx_log_debug1(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0,
-                   "mp4 key frame prefix:%uD", key_prefix);
+                   "mp4 key frame prefix:%uD", *key_prefix);
 
-    return key_prefix;
+    return NGX_OK;
 }
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-cba49dcbf5` → 本草稿移入 `cases/defect/auto-nginx-cba49dcbf5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
