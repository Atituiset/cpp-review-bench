# auto-curl-50cae6e1d5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#15155](https://github.com/curl/curl/pull/15155) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-08-31 |
| track 方向 | defect 候选（polarity=must_find） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15155 (https://github.com/curl/curl/pull/15155)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 516；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15155 multi: Make curl_multi_waitfds consistent with the documentation. :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -493,38 +493,39 @@ void Curl_waitfds_init(struct curl_waitfds *cwfds,
                        unsigned int static_count)
 {
   DEBUGASSERT(cwfds);
-  DEBUGASSERT(static_wfds);
+  DEBUGASSERT(static_wfds || !static_count);
   memset(cwfds, 0, sizeof(*cwfds));
   cwfds->wfds = static_wfds;
   cwfds->count = static_count;
 }
 
-static CURLcode cwfds_add_sock(struct curl_waitfds *cwfds,
-                               curl_socket_t sock, short events)
+static unsigned int cwfds_add_sock(struct curl_waitfds *cwfds,
+                                   curl_socket_t sock, short events)
 {
   int i;
 
   if(cwfds->n <= INT_MAX) {
     for(i = (int)cwfds->n - 1; i >= 0; --i) {
       if(sock == cwfds->wfds[i].fd) {
         cwfds->wfds[i].events |= events;
-        return CURLE_OK;
+        return 0;
       }
     }
   }
   /* not folded, add new entry */
-  if(cwfds->n >= cwfds->count)
-    return CURLE_OUT_OF_MEMORY;
-  cwfds->wfds[cwfds->n].fd = sock;
-  cwfds->wfds[cwfds->n].events = events;
-  ++cwfds->n;
-  return CURLE_OK;
+  if(cwfds->n < cwfds->count) {
+    cwfds->wfds[cwfds->n].fd = sock;
+    cwfds->wfds[cwfds->n].events = events;
+    ++cwfds->n;
+  }
+  return 1;
 }
 
-CURLcode Curl_waitfds_add_ps(struct curl_waitfds *cwfds,
-                             struct easy_pollset *ps)
+unsigned int Curl_waitfds_add_ps(struct curl_waitfds *cwfds,
+                                 struct easy_pollset *ps)
 {
   size_t i;
+  unsigned int need = 0;
 
   DEBUGASSERT(cwfds);
   DEBUGASSERT(ps);
@@ -534,10 +535,8 @@ CURLcode Curl_waitfds_add_ps(struct curl_waitfds *cwfds,
       events |= CURL_WAIT_POLLIN;
     if(ps->actions[i] & CURL_POLL_OUT)
       events |= CURL_WAIT_POLLOUT;
-    if(events) {
-      if(cwfds_add_sock(cwfds, ps->sockets[i], events))
-        return CURLE_OUT_OF_MEMORY;
-    }
+    if(events)
+      need += cwfds_add_sock(cwfds, ps->sockets[i], events);
   }
-  return CURLE_OK;
+  return need;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`memset`
- 大写宏：`CURLE_OK`
- 大写宏：`CURLE_OUT_OF_MEMORY`
- 大写宏：`CURL_POLL_OUT`
- 大写宏：`CURL_WAIT_POLLIN`
- 大写宏：`CURL_WAIT_POLLOUT`
- 大写宏：`DEBUGASSERT`
- 大写宏：`INT_MAX`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-curl-50cae6e1d5` → 本草稿移入 `cases/defect/auto-curl-50cae6e1d5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
