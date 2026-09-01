# auto-nginx-f907a2885d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | nginx/nginx |
| 源 PR | [#1565](https://github.com/nginx/nginx/pull/1565) |
| 许可证 | BSD-2-Clause |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 40 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1565 (https://github.com/nginx/nginx/pull/1565)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 1565 Xslt: xmlCreatePushParserCtxt() error handling :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -279,6 +279,10 @@ ngx_http_xslt_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
 
         if (ngx_http_xslt_add_chunk(r, ctx, cl->buf) != NGX_OK) {
 
+            if (ctx->ctxt == NULL) {
+                return ngx_http_xslt_send(r, ctx, NULL);
+            }
+
             if (ctx->ctxt->myDoc) {
 
 #if (NGX_HTTP_XSLT_REUSE_DTD)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ngx_http_get_module_loc_conf`
- 外部函数：`ngx_http_xslt_add_chunk`
- 外部函数：`ngx_log_debug3`
- 外部函数：`ngx_log_error`
- 外部函数：`xmlAddChild`
- 外部函数：`xmlAddPrevSibling`
- 外部函数：`xmlCopyDtd`
- 外部函数：`xmlCreatePushParserCtxt`
- 外部函数：`xmlCtxtUseOptions`
- 外部函数：`xmlParseChunk`
- 外部函数：`xmlSAX2EntityDecl`
- 大写宏：`DTD`
- 大写宏：`NGX_ERROR`
- 大写宏：`NGX_LOG_DEBUG_HTTP`
- 大写宏：`NGX_LOG_ERR`
- 大写宏：`NGX_LOG_WARN`
- 大写宏：`NGX_OK`
- 大写宏：`NULL`
- 大写宏：`XML`
- 大写宏：`XML_EXTERNAL_GENERAL_PARSED_ENTITY`
- 大写宏：`XML_EXTERNAL_PARAMETER_ENTITY`
- 大写宏：`XML_INTERNAL_GENERAL_ENTITY`
- 大写宏：`XML_INTERNAL_PARAMETER_ENTITY`
- 大写宏：`XML_PARSE_DTDLOAD`
- 大写宏：`XML_PARSE_NOENT`
- 大写宏：`XML_PARSE_NONET`
- 大写宏：`XML_PARSE_NOWARNING`
- 外部类型：`If`
- 外部类型：`This`
- 外部类型：`ngx_array_t`
- 外部类型：`ngx_buf_t`
- 外部类型：`ngx_flag_t`
- 外部类型：`ngx_hash_t`
- 外部类型：`ngx_http_request_t`
- 外部类型：`ngx_http_xslt_filter_ctx_t`
- 外部类型：`ngx_http_xslt_filter_loc_conf_t`
- 外部类型：`ngx_http_xslt_param_t`
- 外部类型：`ngx_http_xslt_sheet_t`
- 外部类型：`ngx_int_t`
- 外部类型：`ngx_uint_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-nginx-f907a2885d` → 本草稿移入 `cases/defect/auto-nginx-f907a2885d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
