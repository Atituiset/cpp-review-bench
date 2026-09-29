# auto-curl-c25f859e0f

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #11384 (https://github.com/curl/curl/pull/11384)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1689；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1686,9 +1686,10 @@ static CURLcode h2_progress_egress(struct Curl_cfilter *cf,
   struct stream_ctx *stream = H2_STREAM_CTX(data);
   int rv = 0;
 
-  if((sweight_wanted(data) != sweight_in_effect(data)) ||
-     (data->set.priority.exclusive != data->state.priority.exclusive) ||
-     (data->set.priority.parent != data->state.priority.parent) ) {
+  if(stream && stream->id > 0 &&
+     ((sweight_wanted(data) != sweight_in_effect(data)) ||
+      (data->set.priority.exclusive != data->state.priority.exclusive) ||
+      (data->set.priority.parent != data->state.priority.parent)) ) {
     /* send new weight and/or dependency */
     nghttp2_priority_spec pri_spec;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-c25f859e0f` → 本草稿移入 `cases/defect/auto-curl-c25f859e0f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
