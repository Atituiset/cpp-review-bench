# auto-nginx-2df0d7efc1

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #938 (https://github.com/nginx/nginx/pull/938)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 242；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -239,7 +239,7 @@ static ngx_command_t  ngx_stream_ssl_commands[] = {
       NULL },
 
     { ngx_string("ssl_ocsp"),
-      NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
+      NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
       ngx_conf_set_enum_slot,
       NGX_STREAM_SRV_CONF_OFFSET,
       offsetof(ngx_stream_ssl_srv_conf_t, ocsp),
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-2df0d7efc1` → 本草稿移入 `cases/defect/auto-nginx-2df0d7efc1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
