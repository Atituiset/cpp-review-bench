# auto-linux-840b23d81c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#d40cb8a12ed25789f39404c5f7968a65e98d89c3](https://github.com/torvalds/linux/commit/d40cb8a12ed25789f39404c5f7968a65e98d89c3) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d40cb8a12ed25789f39404c5f7968a65e98d89c3 (https://github.com/torvalds/linux/commit/d40cb8a12ed25789f39404c5f7968a65e98d89c3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 15（原始 PR diff 行 2943；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d40cb8a12ed25789f39404c5f7968a65e98d89c3 Merge branches 'acpi-cppc-fixes' and 'pm-cpufreq-fixes' :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2081,6 +2081,15 @@ int acpi_cppc_processor_probe(struct acpi_processor *pr)
 			cpc_apply_platform_quirks(gas_t, i - 2,
 						  platform_quirks);
 
+			/* EPP interfaces require the full 0-255 preference range. */
+			if (i - 2 == ENERGY_PERF && gas_t->bit_width < 8 &&
+			    !IS_NULL_REG(gas_t)) {
+				pr_debug("CPU%d: EPP register width %u is not supported\n",
+					 pr->id, gas_t->bit_width);
+				ret = -EOPNOTSUPP;
+				goto out_free;
+			}
+
 			/*
 			 * The PCC Subspace index is encoded inside
 			 * the CPC table entries. The same PCC index
@@ -2940,9 +2949,9 @@ int cppc_get_perf_caps(int cpunum, struct cppc_perf_caps *perf_caps)
 	if (!high || !low || !nom || !ref || !min_nonlinear ||
 	    high > U32_MAX || low > U32_MAX || guaranteed > U32_MAX ||
 	    nom > U32_MAX || ref > U32_MAX || min_nonlinear > U32_MAX ||
-	    high < nom || nom < min_nonlinear || min_nonlinear < low ||
+	    high < min_nonlinear || min_nonlinear < low ||
 	    (CPC_SUPPORTED(guaranteed_reg) &&
-	     (guaranteed < low || guaranteed > nom))) {
+	     (guaranteed < low || guaranteed > high))) {
 		ret = -EFAULT;
 		goto out_err;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pr_info_once`
- 大写宏：`ACPI_ADR_SPACE_SYSTEM_MEMORY`
- 大写宏：`ACPI_TYPE_INTEGER`
- 大写宏：`CPC`
- 大写宏：`CPC_QUIRK_PERF_LIMITED_OWNS_UNIT`
- 大写宏：`EFAULT`
- 大写宏：`PCC`
- 大写宏：`PERF_LIMITED`
- 大写宏：`U32_MAX`
- 外部类型：`Bit`
- 外部类型：`Limited`
- 外部类型：`Performance`
- 外部类型：`Subspace`
- 外部类型：`The`
- 外部类型：`Width`
- 外部类型：`cpc_reg`
- 外部类型：`gas_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-840b23d81c` → 本草稿移入 `cases/defect/auto-linux-840b23d81c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
