# auto-redis-0f645e1245

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15412 (https://github.com/redis/redis/pull/15412)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 324；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -321,7 +321,7 @@ void activeExpireCycle(int type) {
          * too high. Also never repeat a fast cycle for the same period
          * as the fast cycle total duration itself. */
         if (!timelimit_exit &&
-            server.stat_expired_stale_perc < config_cycle_acceptable_stale)
+            server.stat_expired_stale_perc * 100 < config_cycle_acceptable_stale)
             return;
 
         if (start < last_fast_cycle + (long long)config_cycle_fast_duration*2)
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-0f645e1245` → 本草稿移入 `cases/defect/auto-redis-0f645e1245/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
