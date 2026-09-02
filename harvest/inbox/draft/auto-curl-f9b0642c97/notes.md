# auto-curl-f9b0642c97

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#15155](https://github.com/curl/curl/pull/15155) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 57（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15155 (https://github.com/curl/curl/pull/15155)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 929；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15155 multi: Make curl_multi_waitfds consistent with the documentation. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -926,10 +926,10 @@ CURLcode Curl_cpool_add_pollfds(struct cpool *cpool,
   return result;
 }
 
-CURLcode Curl_cpool_add_waitfds(struct cpool *cpool,
-                                struct curl_waitfds *cwfds)
+unsigned int Curl_cpool_add_waitfds(struct cpool *cpool,
+                                    struct curl_waitfds *cwfds)
 {
-  CURLcode result = CURLE_OK;
+  unsigned int need = 0;
 
   CPOOL_LOCK(cpool);
   if(Curl_llist_head(&cpool->shutdowns)) {
@@ -945,14 +945,11 @@ CURLcode Curl_cpool_add_waitfds(struct cpool *cpool,
       Curl_conn_adjust_pollset(cpool->idata, &ps);
       Curl_detach_connection(cpool->idata);
 
-      result = Curl_waitfds_add_ps(cwfds, &ps);
-      if(result)
-        goto out;
+      need += Curl_waitfds_add_ps(cwfds, &ps);
     }
   }
-out:
   CPOOL_UNLOCK(cpool);
-  return result;
+  return need;
 }
 
 static void cpool_perform(struct cpool *cpool)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpool_run_conn_shutdown_handler`
- 外部函数：`disconnect`
- 外部函数：`handler`
- 外部函数：`infof`
- 大写宏：`CCACHE`
- 大写宏：`CURLE_OK`
- 大写宏：`CURL_LOCK_ACCESS_SINGLE`
- 大写宏：`CURL_LOCK_DATA_CONNECT`
- 大写宏：`CURL_SHARE_KEEP_CONNECT`
- 大写宏：`DEBUGASSERT`
- 大写宏：`DEBUGF`
- 大写宏：`EXPIRE_RUN_NOW`
- 大写宏：`FALSE`
- 大写宏：`FIRSTSOCKET`
- 大写宏：`FMT_OFF_T`
- 大写宏：`NEGOTIATE`
- 大写宏：`NTLM`
- 大写宏：`NULL`
- 大写宏：`SECONDARYSOCKET`
- 大写宏：`TRUE`
- 外部类型：`CURLcode`
- 外部类型：`Cleanup`
- 外部类型：`Curl_easy`
- 外部类型：`Curl_llist_node`
- 外部类型：`Make`
- 外部类型：`Not`
- 外部类型：`This`
- 外部类型：`We`
- 外部类型：`connectdata`
- 外部类型：`cpool`
- 外部类型：`curl_waitfds`
- 外部类型：`curltime`
- 外部类型：`timediff_t`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-f9b0642c97` → 本草稿移入 `cases/defect/auto-curl-f9b0642c97/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
