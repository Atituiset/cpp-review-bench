# auto-linux-a950721a79

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7d1471e50943196f909674d2d22eb4196c7df48e](https://github.com/torvalds/linux/commit/7d1471e50943196f909674d2d22eb4196c7df48e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7d1471e50943196f909674d2d22eb4196c7df48e (https://github.com/torvalds/linux/commit/7d1471e50943196f909674d2d22eb4196c7df48e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 23；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7d1471e50943196f909674d2d22eb4196c7df48e Merge tag '9p-for-7.4-rc2' of https://github.com/martinetd/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -20,17 +20,14 @@ int v9fs_cache_session_get_cookie(struct v9fs_session_info *v9ses,
 				  const char *dev_name)
 {
 	struct fscache_volume *vcookie;
-	char *name, *p;
+	char *name;
 
 	name = kasprintf(GFP_KERNEL, "9p,%s,%s",
 			 dev_name, v9ses->cachetag ?: v9ses->aname);
 	if (!name)
 		return -ENOMEM;
 
-	for (p = name; *p; p++)
-		if (*p == '/')
-			*p = ';';
-
+	strreplace(name, '/', ';');
 	vcookie = fscache_acquire_volume(name, NULL, NULL, 0);
 	p9_debug(P9_DEBUG_FSC, "session %p get volume %p (%s)\n",
 		 v9ses, vcookie, name);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fscache_acquire_volume`
- 外部函数：`kasprintf`
- 外部函数：`p`
- 外部函数：`p9_debug`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`NULL`
- 大写宏：`P9_DEBUG_FSC`
- 外部类型：`fscache_volume`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a950721a79` → 本草稿移入 `cases/defect/auto-linux-a950721a79/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
