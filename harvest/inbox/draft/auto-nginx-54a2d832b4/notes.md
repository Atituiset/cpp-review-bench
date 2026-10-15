# auto-nginx-54a2d832b4

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1648 (https://github.com/nginx/nginx/pull/1648)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1110；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -915,7 +915,8 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
                 continue;
             }
 
-            params_len += 1 + key_len + ((val_len > 127) ? 4 : 1) + val_len;
+            params_len += ((key_len > 127) ? 4 : 1) + key_len
+                          + ((val_len > 127) ? 4 : 1) + val_len;
         }
 
         len += params_len;
@@ -1083,7 +1084,7 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
         while (*(uintptr_t *) le.ip) {
 
             lcode = *(ngx_http_script_len_code_pt *) le.ip;
-            key_len = (u_char) lcode(&le);
+            key_len = lcode(&le);
 
             lcode = *(ngx_http_script_len_code_pt *) le.ip;
             skip_empty = lcode(&le);
@@ -1107,13 +1108,23 @@ ngx_http_fastcgi_create_request(ngx_http_request_t *r)
                 continue;
             }
 
-            if (ngx_http_script_check_length(&e, 1 + ((val_len > 127) ? 4 : 1))
+            if (ngx_http_script_check_length(&e,
+                                             ((key_len > 127) ? 4 : 1)
+                                             + ((val_len > 127) ? 4 : 1))
                 != NGX_OK)
             {
                 return NGX_ERROR;
             }
 
-            *e.pos++ = (u_char) key_len;
+            if (key_len > 127) {
+                *e.pos++ = (u_char) (((key_len >> 24) & 0x7f) | 0x80);
+                *e.pos++ = (u_char) ((key_len >> 16) & 0xff);
+                *e.pos++ = (u_char) ((key_len >> 8) & 0xff);
+                *e.pos++ = (u_char) (key_len & 0xff);
+
+            } else {
+                *e.pos++ = (u_char) key_len;
+            }
 
             if (val_len > 127) {
                 *e.pos++ = (u_char) (((val_len >> 24) & 0x7f) | 0x80);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-54a2d832b4` → 本草稿移入 `cases/defect/auto-nginx-54a2d832b4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
