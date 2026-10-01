# auto-nginx-6870fb4203

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #536 (https://github.com/nginx/nginx/pull/536)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -142,7 +142,7 @@ struct ngx_ssl_connection_s {
 #define NGX_SSL_DFLT_BUILTIN_SCACHE  -5
 
 
-#define NGX_SSL_MAX_SESSION_SIZE  4096
+#define NGX_SSL_MAX_SESSION_SIZE  8192
 
 typedef struct ngx_ssl_sess_id_s  ngx_ssl_sess_id_t;
 
@@ -362,4 +362,7 @@ extern int  ngx_ssl_index;
 extern int  ngx_ssl_certificate_name_index;
 
 
+extern u_char  ngx_ssl_session_buffer[NGX_SSL_MAX_SESSION_SIZE];
+
+
 #endif /* _NGX_EVENT_OPENSSL_H_INCLUDED_ */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-6870fb4203` → 本草稿移入 `cases/defect/auto-nginx-6870fb4203/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
