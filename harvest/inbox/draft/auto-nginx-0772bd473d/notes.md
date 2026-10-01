# auto-nginx-0772bd473d

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #353 (https://github.com/nginx/nginx/pull/353)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1032；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1029,7 +1029,7 @@ ngx_quic_handle_payload(ngx_connection_t *c, ngx_quic_header_t *pkt)
          * After receiving a 1-RTT packet, servers MUST discard
          * 0-RTT keys within a short time
          */
-        ngx_quic_discard_ctx(c, ssl_encryption_early_data);
+        ngx_quic_keys_discard(qc->keys, ssl_encryption_early_data);
     }
 
     if (qc->closing) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-0772bd473d` → 本草稿移入 `cases/defect/auto-nginx-0772bd473d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
