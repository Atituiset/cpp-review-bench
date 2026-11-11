# auto-postgres-cd7ac590f1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1d657c6e013e152ed37b42a22ee9f291179ee50f](https://github.com/postgres/postgres/commit/1d657c6e013e152ed37b42a22ee9f291179ee50f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 45（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1d657c6e013e152ed37b42a22ee9f291179ee50f (https://github.com/postgres/postgres/commit/1d657c6e013e152ed37b42a22ee9f291179ee50f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 688；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1d657c6e013e152ed37b42a22ee9f291179ee50f Fix uninitialized cache use in gtsvector_picksplit() :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -678,15 +678,17 @@ gtsvector_picksplit(PG_FUNCTION_ARGS)
 		seed_2 = 2;
 	}
 
+	/* seed_2 can be the last entry, so fill its cache first */
+	maxoff = OffsetNumberNext(maxoff);
+	fillcache(&cache[maxoff], GETENTRY(entryvec, maxoff), siglen);
+
 	/* form initial .. */
 	datum_l = gtsvector_alloc(SIGNKEY | (cache[seed_1].allistrue ? ALLISTRUE : 0),
 							  siglen, cache[seed_1].sign);
 	datum_r = gtsvector_alloc(SIGNKEY | (cache[seed_2].allistrue ? ALLISTRUE : 0),
 							  siglen, cache[seed_2].sign);
 	union_l = GETSIGN(datum_l);
 	union_r = GETSIGN(datum_r);
-	maxoff = OffsetNumberNext(maxoff);
-	fillcache(&cache[maxoff], GETENTRY(entryvec, maxoff), siglen);
 	/* sort before ... */
 	costvector = palloc_array(SPLITCOST, maxoff);
 	for (j = FirstOffsetNumber; j <= maxoff; j = OffsetNumberNext(j))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`header`
- 外部函数：`makesign`
- 外部函数：`memcpy`
- 外部函数：`palloc`
- 外部函数：`palloc_array`
- 大写宏：`BITS_PER_BYTE`
- 大写宏：`BITVECP`
- 大写宏：`CACHESIGN`
- 大写宏：`FLEXIBLE_ARRAY_MEMBER`
- 大写宏：`SET_VARSIZE`
- 大写宏：`SPLITCOST`
- 大写宏：`VARHDRSZ`
- 大写宏：`VARSIZE`
- 外部类型：`FirstOffsetNumber`
- 外部类型：`OffsetNumber`
- 外部类型：`SignTSVector`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-cd7ac590f1` → 本草稿移入 `cases/defect/auto-postgres-cd7ac590f1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
