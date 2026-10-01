# auto-nginx-0787df9529

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1155 (https://github.com/nginx/nginx/pull/1155)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 216；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -213,8 +213,12 @@ ngx_quic_compat_keylog_callback(const SSL *ssl, const char *line)
         com->method->set_read_secret((SSL *) ssl, level, cipher, secret, n);
         com->read_record = 0;
 
-        (void) ngx_quic_compat_set_encryption_secret(c, &com->keys, level,
-                                                     cipher, secret, n);
+        if (ngx_quic_compat_set_encryption_secret(c, &com->keys, level,
+                                                  cipher, secret, n)
+            != NGX_OK)
+        {
+            qc->error = NGX_QUIC_ERR_INTERNAL_ERROR;
+        }
     }
 
     ngx_explicit_memzero(secret, n);
@@ -591,6 +595,10 @@ ngx_quic_compat_create_record(ngx_quic_compat_record_t *rec, ngx_str_t *res)
 
     secret = &rec->keys->secret;
 
+    if (secret->ctx == NULL) {
+        return NGX_ERROR;
+    }
+
     ngx_memcpy(nonce, secret->iv.data, secret->iv.len);
     ngx_quic_compute_nonce(nonce, sizeof(nonce), rec->number);
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-0787df9529` → 本草稿移入 `cases/defect/auto-nginx-0787df9529/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
