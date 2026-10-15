# auto-nginx-db6e3aa188

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1544 (https://github.com/nginx/nginx/pull/1544)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1099,7 +1099,7 @@ ngx_configure_listening_sockets(ngx_cycle_t *cycle)
             }
         }
 
-#elif (NGX_HAVE_IP_DONTFRAG)
+#elif (NGX_HAVE_IPV6_DONTFRAG)
 
         if (ls[i].quic && ls[i].sockaddr->sa_family == AF_INET6) {
             value = 1;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-db6e3aa188` → 本草稿移入 `cases/defect/auto-nginx-db6e3aa188/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
