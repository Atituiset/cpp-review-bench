# auto-curl-7a7002011a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#8835](https://github.com/curl/curl/pull/8835) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-08-31 |
| track 方向 | contract 候选（polarity=must_not_find） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8835 (https://github.com/curl/curl/pull/8835)
- 候选初判 scenario: **None（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 8835 url: remove redundant #ifdefs in allocate_conn() :: FP 修复 PR（静态分析误报/契约安全），contract 轨误报矿候选（待 LLM/人审定）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1770,16 +1770,12 @@ static struct connectdata *allocate_conn(struct Curl_easy *data)
   conn->ssl_config.verifypeer = data->set.ssl.primary.verifypeer;
   conn->ssl_config.verifyhost = data->set.ssl.primary.verifyhost;
   conn->ssl_config.ssl_options = data->set.ssl.primary.ssl_options;
-#ifdef USE_TLS_SRP
-#endif
 #ifndef CURL_DISABLE_PROXY
   conn->proxy_ssl_config.verifystatus =
     data->set.proxy_ssl.primary.verifystatus;
   conn->proxy_ssl_config.verifypeer = data->set.proxy_ssl.primary.verifypeer;
   conn->proxy_ssl_config.verifyhost = data->set.proxy_ssl.primary.verifyhost;
   conn->proxy_ssl_config.ssl_options = data->set.proxy_ssl.primary.ssl_options;
-#ifdef USE_TLS_SRP
-#endif
 #endif
   conn->ip_version = data->set.ipver;
   conn->bits.connect_only = data->set.connect_only;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`CURL_DISABLE_PROXY`
- 大写宏：`USE_TLS_SRP`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-7a7002011a` → 本草稿移入 `cases/contract/auto-curl-7a7002011a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
