# auto-nginx-27d6901dd5

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1209 (https://github.com/nginx/nginx/pull/1209)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 2300；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2297,7 +2297,7 @@ ngx_http_mp4_read_stts_atom(ngx_http_mp4_file_t *mp4, uint64_t atom_data_size)
                    "mp4 time-to-sample entries:%uD", entries);
 
     if (ngx_mp4_atom_data_size(ngx_mp4_stts_atom_t)
-        + entries * sizeof(ngx_mp4_stts_entry_t) > atom_data_size)
+        + (uint64_t) entries * sizeof(ngx_mp4_stts_entry_t) > atom_data_size)
     {
         ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                       "\"%s\" mp4 stts atom too small", mp4->file.name.data);
@@ -2612,7 +2612,7 @@ ngx_http_mp4_read_stss_atom(ngx_http_mp4_file_t *mp4, uint64_t atom_data_size)
     atom->last = atom_table;
 
     if (ngx_mp4_atom_data_size(ngx_http_mp4_stss_atom_t)
-        + entries * sizeof(uint32_t) > atom_data_size)
+        + (uint64_t) entries * sizeof(uint32_t) > atom_data_size)
     {
         ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                       "\"%s\" mp4 stss atom too small", mp4->file.name.data);
@@ -2817,7 +2817,7 @@ ngx_http_mp4_read_ctts_atom(ngx_http_mp4_file_t *mp4, uint64_t atom_data_size)
     atom->last = atom_table;
 
     if (ngx_mp4_atom_data_size(ngx_mp4_ctts_atom_t)
-        + entries * sizeof(ngx_mp4_ctts_entry_t) > atom_data_size)
+        + (uint64_t) entries * sizeof(ngx_mp4_ctts_entry_t) > atom_data_size)
     {
         ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                       "\"%s\" mp4 ctts atom too small", mp4->file.name.data);
@@ -2999,7 +2999,7 @@ ngx_http_mp4_read_stsc_atom(ngx_http_mp4_file_t *mp4, uint64_t atom_data_size)
                    "sample-to-chunk entries:%uD", entries);
 
     if (ngx_mp4_atom_data_size(ngx_mp4_stsc_atom_t)
-        + entries * sizeof(ngx_mp4_stsc_entry_t) > atom_data_size)
+        + (uint64_t) entries * sizeof(ngx_mp4_stsc_entry_t) > atom_data_size)
     {
         ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                       "\"%s\" mp4 stsc atom too small", mp4->file.name.data);
@@ -3393,7 +3393,7 @@ ngx_http_mp4_read_stsz_atom(ngx_http_mp4_file_t *mp4, uint64_t atom_data_size)
 
     if (size == 0) {
         if (ngx_mp4_atom_data_size(ngx_mp4_stsz_atom_t)
-            + entries * sizeof(uint32_t) > atom_data_size)
+            + (uint64_t) entries * sizeof(uint32_t) > atom_data_size)
         {
             ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                           "\"%s\" mp4 stsz atom too small",
@@ -3552,7 +3552,7 @@ ngx_http_mp4_read_stco_atom(ngx_http_mp4_file_t *mp4, uint64_t atom_data_size)
     ngx_log_debug1(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0, "chunks:%uD", entries);
 
     if (ngx_mp4_atom_data_size(ngx_mp4_stco_atom_t)
-        + entries * sizeof(uint32_t) > atom_data_size)
+        + (uint64_t) entries * sizeof(uint32_t) > atom_data_size)
     {
         ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                       "\"%s\" mp4 stco atom too small", mp4->file.name.data);
@@ -3768,7 +3768,7 @@ ngx_http_mp4_read_co64_atom(ngx_http_mp4_file_t *mp4, uint64_t atom_data_size)
     ngx_log_debug1(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0, "chunks:%uD", entries);
 
     if (ngx_mp4_atom_data_size(ngx_mp4_co64_atom_t)
-        + entries * sizeof(uint64_t) > atom_data_size)
+        + (uint64_t) entries * sizeof(uint64_t) > atom_data_size)
     {
         ngx_log_error(NGX_LOG_ERR, mp4->file.log, 0,
                       "\"%s\" mp4 co64 atom too small", mp4->file.name.data);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-27d6901dd5` → 本草稿移入 `cases/defect/auto-nginx-27d6901dd5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
