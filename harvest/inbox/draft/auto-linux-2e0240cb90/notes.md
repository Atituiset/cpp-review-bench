# auto-linux-2e0240cb90

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4843f6873f7c4835e62f2c73ba0fd538e57a2f22](https://github.com/torvalds/linux/commit/4843f6873f7c4835e62f2c73ba0fd538e57a2f22) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4843f6873f7c4835e62f2c73ba0fd538e57a2f22 (https://github.com/torvalds/linux/commit/4843f6873f7c4835e62f2c73ba0fd538e57a2f22)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 142；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4843f6873f7c4835e62f2c73ba0fd538e57a2f22 Merge tag 'sysctl-7.04-fixes-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/sysctl/sysctl :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -137,11 +137,19 @@ static int sysctl_k2u_int_conv_userhz(bool *negp, ulong *u_ptr, const int *k_ptr
 	return proc_int_k2u_conv_kop(u_ptr, k_ptr, negp, sysctl_jiffies_to_clock_t);
 }
 
+/*
+ * Calc ms <-> jiffies conversions for sysctls in unsigned long arithmetic.
+ * Think hard about using the non-sysctl converters (msecs_to_jiffies &
+ * jiffies_to_msecs) as the interaction between ulong and uint is not trivial.
+ */
 static ulong sysctl_msecs_to_jiffies(const ulong val)
 {
-	if (val > jiffies_to_msecs(MAX_JIFFY_OFFSET))
-		return MAX_JIFFY_OFFSET;
-	return msecs_to_jiffies(val);
+	ulong secs = val / MSEC_PER_SEC;
+
+	if (secs >= ULONG_MAX / HZ)
+		return ULONG_MAX;
+
+	return secs * HZ + DIV_ROUND_UP((val % MSEC_PER_SEC) * HZ, MSEC_PER_SEC);
 }
 
 static int sysctl_u2k_int_conv_ms(const bool *negp, const ulong *u_ptr, int *k_ptr)
@@ -151,7 +159,12 @@ static int sysctl_u2k_int_conv_ms(const bool *negp, const ulong *u_ptr, int *k_p
 
 static ulong sysctl_jiffies_to_msecs(const ulong val)
 {
-	return jiffies_to_msecs(val);
+	ulong secs = val / HZ;
+
+	if (secs >= ULONG_MAX / MSEC_PER_SEC)
+		return ULONG_MAX;
+
+	return secs * MSEC_PER_SEC + (val % HZ) * MSEC_PER_SEC / HZ;
 }
 
 static int sysctl_k2u_int_conv_ms(bool *negp, ulong *u_ptr, const int *k_ptr)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`jiffies_to_clock_t`
- 外部函数：`jiffies_to_msecs`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`proc_int_k2u_conv_kop`
- 外部函数：`sysctl_jiffies_to_msecs`
- 大写宏：`MAX_JIFFY_OFFSET`
- 外部类型：`sysctl_jiffies_to_clock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-2e0240cb90` → 本草稿移入 `cases/defect/auto-linux-2e0240cb90/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
