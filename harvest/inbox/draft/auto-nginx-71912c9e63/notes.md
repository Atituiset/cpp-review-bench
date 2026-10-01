# auto-nginx-71912c9e63

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1592 (https://github.com/nginx/nginx/pull/1592)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 697；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -694,7 +694,7 @@ ngx_quic_log_frame(ngx_log_t *log, ngx_quic_frame_t *f, ngx_uint_t tx)
     case NGX_QUIC_FT_ACK:
     case NGX_QUIC_FT_ACK_ECN:
 
-        p = ngx_slprintf(p, last, "ACK n:%ui delay:%uL ",
+        p = ngx_slprintf(p, last, "ACK n:%uL delay:%uL ",
                          f->u.ack.range_count, f->u.ack.delay);
 
         if (f->data) {
@@ -758,7 +758,7 @@ ngx_quic_log_frame(ngx_log_t *log, ngx_quic_frame_t *f, ngx_uint_t tx)
 
     case NGX_QUIC_FT_CONNECTION_CLOSE:
     case NGX_QUIC_FT_CONNECTION_CLOSE_APP:
-        p = ngx_slprintf(p, last, "CONNECTION_CLOSE%s err:%ui",
+        p = ngx_slprintf(p, last, "CONNECTION_CLOSE%s err:%uL",
                          f->type == NGX_QUIC_FT_CONNECTION_CLOSE ? "" : "_APP",
                          f->u.close.error_code);
 
@@ -767,7 +767,7 @@ ngx_quic_log_frame(ngx_log_t *log, ngx_quic_frame_t *f, ngx_uint_t tx)
         }
 
         if (f->type == NGX_QUIC_FT_CONNECTION_CLOSE) {
-            p = ngx_slprintf(p, last, " ft:%ui", f->u.close.frame_type);
+            p = ngx_slprintf(p, last, " ft:%uL", f->u.close.frame_type);
         }
 
         break;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-71912c9e63` → 本草稿移入 `cases/defect/auto-nginx-71912c9e63/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
