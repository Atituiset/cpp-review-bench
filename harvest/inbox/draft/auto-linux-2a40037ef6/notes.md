# auto-linux-2a40037ef6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c339f4010dea8b0d754f84ace70cd1b089878ef8](https://github.com/torvalds/linux/commit/c339f4010dea8b0d754f84ace70cd1b089878ef8) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 46 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c339f4010dea8b0d754f84ace70cd1b089878ef8 (https://github.com/torvalds/linux/commit/c339f4010dea8b0d754f84ace70cd1b089878ef8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 517；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c339f4010dea8b0d754f84ace70cd1b089878ef8 Merge tag 'platform-drivers-x86-v7.4-1' of git://git.kernel.org/pub/scm/linux/kernel/git/pdx86/platform-drivers-x86 :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -514,7 +514,7 @@ static int aspire_ec_probe(struct i2c_client *client)
 					aspire_ec_irq_handler, IRQF_ONESHOT,
 					dev_name(dev), ec);
 	if (ret)
-		return dev_err_probe(dev, ret, "Failed to request irq\n");
+		return ret;
 
 	return 0;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_err`
- 外部函数：`dev_err_probe`
- 外部函数：`dev_name`
- 外部函数：`dev_warn`
- 外部函数：`drm_bridge_hpd_notify`
- 外部函数：`i2c_smbus_read_byte_data`
- 外部函数：`i2c_smbus_read_i2c_block_data`
- 外部函数：`i2c_smbus_write_byte_data`
- 外部函数：`i2c_smbus_write_i2c_block_data`
- 外部函数：`input_report_switch`
- 外部函数：`input_sync`
- 外部函数：`power_supply_changed`
- 外部函数：`usleep_range`
- 大写宏：`ACPI`
- 大写宏：`ADP1`
- 大写宏：`BAT1`
- 大写宏：`BIT`
- 大写宏：`ERR_PTR`
- 大写宏：`I2C3`
- 大写宏：`IRQF_ONESHOT`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`LID0`
- 大写宏：`SW_LID`
- 外部类型：`Acer`
- 外部类型：`Aspire`
- 外部类型：`Change`
- 外部类型：`Display`
- 外部类型：`Failed`
- 外部类型：`Here`
- 外部类型：`Information`
- 外部类型：`It`
- 外部类型：`No`
- 外部类型：`Seems`
- 外部类型：`So`
- 外部类型：`Spin`
- 外部类型：`Status`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Thus`
- 外部类型：`Unknown`
- 外部类型：`drm_bridge`
- 外部类型：`i2c_client`
- 外部类型：`input_dev`
- 外部类型：`irqreturn_t`
- 外部类型：`power_supply`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-2a40037ef6` → 本草稿移入 `cases/defect/auto-linux-2a40037ef6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
