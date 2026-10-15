# auto-curl-0f1d42e63c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#16601](https://github.com/curl/curl/pull/16601) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16601 (https://github.com/curl/curl/pull/16601)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2148；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16601 setopt: setting PROXYUSERPWD after PROXYUSERNAME/PASSWORD is fine :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2145,12 +2145,16 @@ static CURLcode setopt_cptr(struct Curl_easy *data, CURLoption option,
     result = setstropt_userpwd(ptr, &u, &p);
 
     /* URL decode the components */
-    if(!result && u)
+    if(!result && u) {
+      Curl_safefree(data->set.str[STRING_PROXYUSERNAME]);
       result = Curl_urldecode(u, 0, &data->set.str[STRING_PROXYUSERNAME], NULL,
                               REJECT_ZERO);
-    if(!result && p)
+    }
+    if(!result && p) {
+      Curl_safefree(data->set.str[STRING_PROXYPASSWORD]);
       result = Curl_urldecode(p, 0, &data->set.str[STRING_PROXYPASSWORD], NULL,
                               REJECT_ZERO);
+    }
     free(u);
     free(p);
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`free`
- 外部函数：`strlen`
- 大写宏：`CURLE_BAD_FUNCTION_ARGUMENT`
- 大写宏：`CURLE_OK`
- 大写宏：`CURL_MAX_INPUT_LENGTH`
- 大写宏：`DEBUGASSERT`
- 大写宏：`NULL`
- 大写宏：`REJECT_ZERO`
- 大写宏：`STRING_PROXYPASSWORD`
- 大写宏：`STRING_PROXYUSERNAME`
- 大写宏：`URL`
- 外部类型：`CURLcode`
- 外部类型：`It`
- 外部类型：`Parse`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-0f1d42e63c` → 本草稿移入 `cases/defect/auto-curl-0f1d42e63c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
