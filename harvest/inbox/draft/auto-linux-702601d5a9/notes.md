# auto-linux-702601d5a9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#54975c6523dc77862c25deeb5fa395990b8d3fc4](https://github.com/torvalds/linux/commit/54975c6523dc77862c25deeb5fa395990b8d3fc4) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 71 |
| 编译错误数（gcc syntax-only） | 132（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #54975c6523dc77862c25deeb5fa395990b8d3fc4 (https://github.com/torvalds/linux/commit/54975c6523dc77862c25deeb5fa395990b8d3fc4)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 589；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 54975c6523dc77862c25deeb5fa395990b8d3fc4 Merge tag 'hwmon-for-v7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -528,8 +528,6 @@ static void gpio_fan_stop(void *data)
 	mutex_lock(&fan_data->lock);
 	set_fan_speed(data, 0);
 	mutex_unlock(&fan_data->lock);
-
-	pm_runtime_disable(fan_data->dev);
 }
 
 static int gpio_fan_probe(struct platform_device *pdev)
@@ -556,13 +554,24 @@ static int gpio_fan_probe(struct platform_device *pdev)
 		return dev_err_probe(dev, PTR_ERR(fan_data->supply),
 				     "Failed to get fan-supply");
 
+	pm_runtime_set_suspended(dev);
+	err = devm_pm_runtime_enable(dev);
+	if (err)
+		return err;
+
 	/* Configure control GPIOs if available. */
 	if (fan_data->gpios && fan_data->num_gpios > 0) {
 		if (!fan_data->speed || fan_data->num_speed <= 1)
 			return -EINVAL;
 		err = fan_ctrl_init(fan_data);
 		if (err)
 			return err;
+		/* If current GPIO state is active, mark RPM as active as well */
+		if (fan_data->speed_index > 0) {
+			err = pm_runtime_resume_and_get(dev);
+			if (err)
+				return err;
+		}
 		err = devm_add_action_or_reset(dev, gpio_fan_stop, fan_data);
 		if (err)
 			return err;
@@ -583,17 +592,6 @@ static int gpio_fan_probe(struct platform_device *pdev)
 			return err;
 	}
 
-	pm_runtime_set_suspended(&pdev->dev);
-	pm_runtime_enable(&pdev->dev);
-	/* If current GPIO state is active, mark RPM as active as well */
-	if (fan_data->speed_index > 0) {
-		int ret;
-
-		ret = pm_runtime_resume_and_get(&pdev->dev);
-		if (ret)
-			return ret;
-	}
-
 	/* Optional cooling device register for Device tree platforms */
 	fan_data->cdev = devm_thermal_of_child_cooling_device_register(dev, np,
 								       "gpio-fan",
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`container_of`
- 外部函数：`dev_err`
- 外部函数：`dev_err_probe`
- 外部函数：`dev_info`
- 外部函数：`dev_warn`
- 外部函数：`devm_add_action_or_reset`
- 外部函数：`devm_gpiod_get_index`
- 外部函数：`devm_gpiod_get_optional`
- 外部函数：`devm_hwmon_device_register_with_groups`
- 外部函数：`devm_kzalloc_obj`
- 外部函数：`devm_kzalloc_objs`
- 外部函数：`devm_regulator_get`
- 外部函数：`devm_request_irq`
- 外部函数：`devm_thermal_of_child_cooling_device_register`
- 外部函数：`devm_work_autocancel`
- 外部函数：`gpiod_count`
- 外部函数：`gpiod_direction_output`
- 外部函数：`gpiod_get_value_cansleep`
- 外部函数：`gpiod_set_value_cansleep`
- 外部函数：`gpiod_to_irq`
- 外部函数：`irq_set_irq_type`
- 外部函数：`kobject_uevent`
- 外部函数：`mutex_init`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`of_find_property`
- 外部函数：`of_prop_next_u32`
- 外部函数：`platform_set_drvdata`
- 外部函数：`pm_runtime_disable`
- 外部函数：`pm_runtime_enable`
- 外部函数：`pm_runtime_put_sync`
- 外部函数：`pm_runtime_resume_and_get`
- 外部函数：`pm_runtime_set_suspended`
- 外部函数：`schedule_work`
- 外部函数：`sysfs_notify`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`ENOSYS`
- 大写宏：`GPIO`
- 大写宏：`GPIOD_ASIS`
- 大写宏：`GPIOD_IN`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`IRQ_TYPE_EDGE_BOTH`
- 大写宏：`IS_ERR`
- 大写宏：`KOBJ_CHANGE`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 大写宏：`RPM`
- 外部类型：`Alarm`
- 外部类型：`Configure`
- 外部类型：`Device`
- 外部类型：`Enable`
- 外部类型：`Failed`
- 外部类型：`Fill`
- 外部类型：`GPIOs`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Make`
- 外部类型：`Optional`
- 外部类型：`Populate`
- 外部类型：`Speed`
- 外部类型：`The`
- 外部类型：`device`
- 外部类型：`device_node`
- 外部类型：`gpio_desc`
- 外部类型：`gpio_fan_data`
- 外部类型：`gpio_fan_speed`
- 外部类型：`irqreturn_t`
- 外部类型：`platform_device`
- 外部类型：`property`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-702601d5a9` → 本草稿移入 `cases/defect/auto-linux-702601d5a9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
