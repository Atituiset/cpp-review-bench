# auto-nginx-67af8097b5

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #345 (https://github.com/nginx/nginx/pull/345)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 3179；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3176,7 +3176,10 @@ ngx_http_mp4_crop_stsc_data(ngx_http_mp4_file_t *mp4,
 
         start_sample -= n;
 
-        prev_samples = samples;
+        if (next_chunk > chunk) {
+            prev_samples = samples;
+        }
+
         chunk = next_chunk;
         samples = ngx_mp4_get_32value(entry->samples);
         id = ngx_mp4_get_32value(entry->id);
@@ -3186,6 +3189,13 @@ ngx_http_mp4_crop_stsc_data(ngx_http_mp4_file_t *mp4,
 
     next_chunk = trak->chunks + 1;
 
+    if (next_chunk < chunk) {
+        ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
+                      "unordered mp4 stsc chunks in \"%s\"",
+                      mp4->file.name.data);
+        return NGX_ERROR;
+    }
+
     ngx_log_debug4(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0,
                    "sample:%uD, chunk:%uD, chunks:%uD, samples:%uD",
                    start_sample, chunk, next_chunk - chunk, samples);
@@ -3211,6 +3221,12 @@ ngx_http_mp4_crop_stsc_data(ngx_http_mp4_file_t *mp4,
         return NGX_ERROR;
     }
 
+    if (chunk == 0) {
+        ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
+                      "zero chunk in \"%s\"", mp4->file.name.data);
+        return NGX_ERROR;
+    }
+
     target_chunk = chunk - 1;
     target_chunk += start_sample / samples;
     chunk_samples = start_sample % samples;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-67af8097b5` → 本草稿移入 `cases/defect/auto-nginx-67af8097b5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
