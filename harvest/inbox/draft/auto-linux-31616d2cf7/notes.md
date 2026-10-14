# auto-linux-31616d2cf7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#d77bd0d469e1923e587bb919301c0bb29093e30a](https://github.com/torvalds/linux/commit/d77bd0d469e1923e587bb919301c0bb29093e30a) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d77bd0d469e1923e587bb919301c0bb29093e30a (https://github.com/torvalds/linux/commit/d77bd0d469e1923e587bb919301c0bb29093e30a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 307；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d77bd0d469e1923e587bb919301c0bb29093e30a Merge tag 'for-v7.3-rc' of git://git.kernel.org/pub/scm/linux/kernel/git/sre/linux-power-supply :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -304,7 +304,7 @@ static int ds2780_get_capacity(struct ds2780_device_info *dev_info,
 		return ret;
 
 	*capacity = raw;
-	return raw;
+	return 0;
 }
 
 static int ds2780_get_status(struct ds2780_device_info *dev_info, int *status)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_err`
- 外部函数：`w1_ds2780_io`
- 大写宏：`CURRENT_AVG`
- 大写宏：`CURRENT_NOW`
- 大写宏：`DS2780_CURRENT_MSB_REG`
- 大写宏：`DS2780_IAVG_MSB_REG`
- 大写宏：`DS2780_RARC_REG`
- 大写宏：`DS2780_RSNSP_REG`
- 大写宏：`EINVAL`
- 大写宏：`LSB`
- 大写宏：`MSB`
- 大写宏：`POWER_SUPPLY_STATUS_CHARGING`
- 大写宏：`POWER_SUPPLY_STATUS_DISCHARGING`
- 大写宏：`POWER_SUPPLY_STATUS_FULL`
- 大写宏：`POWER_SUPPLY_STATUS_NOT_CHARGING`
- 外部类型：`Bits`
- 外部类型：`Sign`
- 外部类型：`The`
- 外部类型：`device`
- 外部类型：`power_supply`
- 外部类型：`power_supply_desc`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-31616d2cf7` → 本草稿移入 `cases/defect/auto-linux-31616d2cf7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
