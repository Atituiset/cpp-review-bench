# auto-nginx-974a2b9a03

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #631 (https://github.com/nginx/nginx/pull/631)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 118；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -115,10 +115,11 @@ ngx_http_v2_header_filter(ngx_http_request_t *r)
     ngx_http_core_srv_conf_t  *cscf;
     u_char                     addr[NGX_SOCKADDR_STRLEN];
 
-    static const u_char nginx[5] = "\x84\xaa\x63\x55\xe7";
+    static const u_char nginx[5] = { 0x84, 0xaa, 0x63, 0x55, 0xe7 };
 #if (NGX_HTTP_GZIP)
-    static const u_char accept_encoding[12] =
-        "\x8b\x84\x84\x2d\x69\x5b\x05\x44\x3c\x86\xaa\x6f";
+    static const u_char accept_encoding[12] = {
+        0x8b, 0x84, 0x84, 0x2d, 0x69, 0x5b, 0x05, 0x44, 0x3c, 0x86, 0xaa, 0x6f
+    };
 #endif
 
     static size_t nginx_ver_len = ngx_http_v2_literal_size(NGINX_VER);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-974a2b9a03` → 本草稿移入 `cases/defect/auto-nginx-974a2b9a03/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
