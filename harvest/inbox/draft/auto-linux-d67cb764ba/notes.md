# auto-linux-d67cb764ba

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#75099ce6829ff89464cb7dd32471024c6c17c1de](https://github.com/torvalds/linux/commit/75099ce6829ff89464cb7dd32471024c6c17c1de) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 50（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #75099ce6829ff89464cb7dd32471024c6c17c1de (https://github.com/torvalds/linux/commit/75099ce6829ff89464cb7dd32471024c6c17c1de)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 370；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 75099ce6829ff89464cb7dd32471024c6c17c1de Merge tag 'pm-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/rafael/linux-pm :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -365,14 +365,25 @@ int acpi_bus_init_power(struct acpi_device *device)
 	 * fails, ACPI power management will not be used for the device going
 	 * forward.
 	 */
+	if (!acpi_device_is_present(device)) {
+		acpi_handle_debug(device->handle,
+				  "Device not present, ACPI PM disabled\n");
+		result = -ENXIO;
+		goto fail;
+	}
+
 	result = acpi_device_init_power(device);
 	if (result) {
-		device->flags.power_manageable = 0;
-		device->power.state = ACPI_D_STATE_INVALID;
 		acpi_handle_info(device->handle,
 				 "Initial power state undetermined, ACPI PM disabled\n");
+		goto fail;
 	}
 
+	return 0;
+
+fail:
+	device->flags.power_manageable = 0;
+	device->power.state = ACPI_D_STATE_INVALID;
 	return result;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`acpi_dev_parent`
- 外部函数：`acpi_evaluate_integer`
- 外部函数：`acpi_evaluate_object`
- 外部函数：`acpi_handle_debug`
- 外部函数：`acpi_handle_info`
- 外部函数：`acpi_power_get_inferred_state`
- 外部函数：`acpi_power_on_resources`
- 大写宏：`ACPI`
- 大写宏：`ACPI_D_STATE_COUNT`
- 大写宏：`ACPI_FAILURE`
- 大写宏：`ACPI_STATE_D0`
- 大写宏：`ACPI_STATE_D1`
- 大写宏：`ACPI_STATE_D2`
- 大写宏：`ACPI_STATE_D3_COLD`
- 大写宏：`ACPI_STATE_D3_HOT`
- 大写宏：`ACPI_STATE_UNKNOWN`
- 大写宏：`BIOS`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`NULL`
- 大写宏：`P870`
- 大写宏：`TBD`
- 外部类型：`Cross`
- 外部类型：`D3cold`
- 外部类型：`D3hot`
- 外部类型：`For`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Initial`
- 外部类型：`No`
- 外部类型：`Non`
- 外部类型：`Power`
- 外部类型：`Reference`
- 外部类型：`The`
- 外部类型：`Toshiba`
- 外部类型：`We`
- 外部类型：`acpi_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d67cb764ba` → 本草稿移入 `cases/defect/auto-linux-d67cb764ba/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
