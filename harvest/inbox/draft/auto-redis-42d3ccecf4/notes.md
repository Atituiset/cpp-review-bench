# auto-redis-42d3ccecf4

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15532 (https://github.com/redis/redis/pull/15532)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -157,7 +157,7 @@ void crcspeed16big_init(crcfn16 fn, uint16_t big_table[8][256]) {
  * reasonable for Intel CPUs made since 2010. Please adjust as necessary if
  * or when your CPU has more load / execute units. We've written benchmark code
  * to help you tune your platform, see crc64Test. */
-#if defined(__i386__) || defined(__X86_64__)
+#if defined(__i386__) || defined(__X86_64__) || defined(__x86_64__)
 static size_t CRC64_TRI_CUTOFF = (2*1024);
 static size_t CRC64_DUAL_CUTOFF = (128);
 #else
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-42d3ccecf4` → 本草稿移入 `cases/defect/auto-redis-42d3ccecf4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
