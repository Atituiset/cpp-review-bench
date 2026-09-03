# auto-nginx-6b3c4b4313

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #479 (https://github.com/nginx/nginx/pull/479)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 414；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -411,7 +411,7 @@ ngx_quic_send_segments(ngx_connection_t *c, u_char *buf, size_t len,
     ngx_memzero(msg_control, sizeof(msg_control));
 
     iov.iov_len = len;
-    iov.iov_base = buf;
+    iov.iov_base = (void *) buf;
 
     msg.msg_iov = &iov;
     msg.msg_iovlen = 1;
@@ -699,7 +699,7 @@ ngx_quic_send(ngx_connection_t *c, u_char *buf, size_t len,
     ngx_memzero(&msg, sizeof(struct msghdr));
 
     iov.iov_len = len;
-    iov.iov_base = buf;
+    iov.iov_base = (void *) buf;
 
     msg.msg_iov = &iov;
     msg.msg_iovlen = 1;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-6b3c4b4313` → 本草稿移入 `cases/defect/auto-nginx-6b3c4b4313/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
