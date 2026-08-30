# auto-nginx-8d5b6ac696

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #634 (https://github.com/nginx/nginx/pull/634)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -19,7 +19,9 @@
 #include <openssl/bn.h>
 #include <openssl/conf.h>
 #include <openssl/crypto.h>
+#ifndef OPENSSL_NO_DH
 #include <openssl/dh.h>
+#endif
 #ifndef OPENSSL_NO_ENGINE
 #include <openssl/engine.h>
 #endif
@@ -83,6 +85,17 @@
 #endif
 
 
+#ifdef OPENSSL_NO_DEPRECATED_3_4
+#define SSL_SESSION_get_time(s)      SSL_SESSION_get_time_ex(s)
+#define SSL_SESSION_set_time(s, t)   SSL_SESSION_set_time_ex(s, t)
+#endif
+
+
+#ifdef OPENSSL_NO_DEPRECATED_3_0
+#define EVP_CIPHER_CTX_cipher(c)     EVP_CIPHER_CTX_get0_cipher(c)
+#endif
+
+
 typedef struct ngx_ssl_ocsp_s   ngx_ssl_ocsp_t;
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-8d5b6ac696` → 本草稿移入 `cases/defect/auto-nginx-8d5b6ac696/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
