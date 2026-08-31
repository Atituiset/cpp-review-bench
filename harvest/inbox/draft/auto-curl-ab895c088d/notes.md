# auto-curl-ab895c088d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#15155](https://github.com/curl/curl/pull/15155) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-08-31 |
| track 方向 | contract 候选（polarity=must_not_find） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15155 (https://github.com/curl/curl/pull/15155)
- 候选初判 scenario: **None（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15155 multi: Make curl_multi_waitfds consistent with the documentation. :: FP 修复 PR（静态分析误报/契约安全），contract 轨误报矿候选（待 LLM/人审定）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1200,8 +1200,9 @@ CURLMcode curl_multi_waitfds(CURLM *m,
   CURLMcode result = CURLM_OK;
   struct Curl_llist_node *e;
   struct Curl_multi *multi = m;
+  unsigned int need = 0;
 
-  if(!ufds)
+  if(!ufds && (size || !fd_count))
     return CURLM_BAD_FUNCTION_ARGUMENT;
 
   if(!GOOD_MULTI_HANDLE(multi))
@@ -1214,20 +1215,17 @@ CURLMcode curl_multi_waitfds(CURLM *m,
   for(e = Curl_llist_head(&multi->process); e; e = Curl_node_next(e)) {
     struct Curl_easy *data = Curl_node_elem(e);
     multi_getsock(data, &data->last_poll);
-    if(Curl_waitfds_add_ps(&cwfds, &data->last_poll)) {
-      result = CURLM_OUT_OF_MEMORY;
-      goto out;
-    }
+    need += Curl_waitfds_add_ps(&cwfds, &data->last_poll);
   }
 
-  if(Curl_cpool_add_waitfds(&multi->cpool, &cwfds)) {
+  need += Curl_cpool_add_waitfds(&multi->cpool, &cwfds);
+
+  if(need != cwfds.n && ufds) {
     result = CURLM_OUT_OF_MEMORY;
-    goto out;
   }
 
-out:
   if(fd_count)
-    *fd_count = cwfds.n;
+    *fd_count = need;
   return result;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`multi_getsock`
- 大写宏：`CURLM_BAD_FUNCTION_ARGUMENT`
- 大写宏：`CURLM_OK`
- 大写宏：`CURLM_OUT_OF_MEMORY`
- 大写宏：`GOOD_MULTI_HANDLE`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## 为什么契约安全

（模板，移植者填：此处报出为什么是误报？豁免契约是什么？与 contract.yaml 的 contract.name / exemption_pattern 对应。）

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-curl-ab895c088d` → 本草稿移入 `cases/contract/auto-curl-ab895c088d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
