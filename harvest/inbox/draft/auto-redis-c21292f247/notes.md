# auto-redis-c21292f247

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14661 (https://github.com/redis/redis/pull/14661)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 899；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -896,7 +896,7 @@ NULL
             addReplyError(c,"Wrong protocol type name. Please use one of the following: string|integer|double|bignum|null|array|set|map|attrib|push|verbatim|true|false");
         }
     } else if (!strcasecmp(c->argv[1]->ptr,"sleep") && c->argc == 3) {
-        double dtime = fast_float_strtod(c->argv[2]->ptr,NULL);
+        double dtime = fast_float_strtod(c->argv[2]->ptr,sdslen(c->argv[2]->ptr),NULL);
         long long utime = dtime*1000000;
         struct timespec tv;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-c21292f247` → 本草稿移入 `cases/defect/auto-redis-c21292f247/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
