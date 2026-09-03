# auto-nginx-9ba5fb7f13

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #247 (https://github.com/nginx/nginx/pull/247)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1511,6 +1511,28 @@ ngx_http_uwsgi_create_loc_conf(ngx_conf_t *cf)
         return NULL;
     }
 
+    /*
+     * set by ngx_pcalloc():
+     *
+     *     conf->upstream.bufs.num = 0;
+     *     conf->upstream.ignore_headers = 0;
+     *     conf->upstream.next_upstream = 0;
+     *     conf->upstream.cache_zone = NULL;
+     *     conf->upstream.cache_use_stale = 0;
+     *     conf->upstream.cache_methods = 0;
+     *     conf->upstream.temp_path = NULL;
+     *     conf->upstream.hide_headers_hash = { NULL, 0 };
+     *     conf->upstream.store_lengths = NULL;
+     *     conf->upstream.store_values = NULL;
+     *
+     *     conf->uwsgi_string = { 0, NULL };
+     *     conf->ssl = 0;
+     *     conf->ssl_protocols = 0;
+     *     conf->ssl_ciphers = { 0, NULL };
+     *     conf->ssl_trusted_certificate = { 0, NULL };
+     *     conf->ssl_crl = { 0, NULL };
+     */
+
     conf->modifier1 = NGX_CONF_UNSET_UINT;
     conf->modifier2 = NGX_CONF_UNSET_UINT;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-9ba5fb7f13` → 本草稿移入 `cases/defect/auto-nginx-9ba5fb7f13/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
