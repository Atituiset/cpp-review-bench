# auto-linux-a7c0cb4ae2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b152d7176339f11f05457799be609268e1a61cf2](https://github.com/torvalds/linux/commit/b152d7176339f11f05457799be609268e1a61cf2) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 51 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b152d7176339f11f05457799be609268e1a61cf2 (https://github.com/torvalds/linux/commit/b152d7176339f11f05457799be609268e1a61cf2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 130；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b152d7176339f11f05457799be609268e1a61cf2 Merge tag 'pinctrl-v7.4-2' of git://git.kernel.org/pub/scm/linux/kernel/git/linusw/linux-pinctrl :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -14,6 +14,7 @@
 #include <linux/scmi_protocol.h>
 #include <linux/slab.h>
 #include <linux/types.h>
+#include <linux/units.h>
 
 #include <linux/pinctrl/machine.h>
 #include <linux/pinctrl/pinconf.h>
@@ -127,11 +128,8 @@ static int pinctrl_scmi_map_pinconf_type(enum pin_config_param param,
 	case PIN_CONFIG_INPUT_ENABLE:
 		*type = SCMI_PIN_INPUT_MODE;
 		break;
-	case PIN_CONFIG_INPUT_SCHMITT:
-		*type = SCMI_PIN_INPUT_SCHMITT;
-		break;
 	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
-		*type = SCMI_PIN_INPUT_MODE;
+		*type = SCMI_PIN_INPUT_SCHMITT;
 		break;
 	case PIN_CONFIG_MODE_LOW_POWER:
 		*type = SCMI_PIN_LOW_POWER_MODE;
@@ -155,6 +153,15 @@ static int pinctrl_scmi_map_pinconf_type(enum pin_config_param param,
 	return 0;
 }
 
+static u32 pinctrl_scmi_map_pinconf_arg_get(enum pin_config_param param,
+					    u32 value)
+{
+	if (param == PIN_CONFIG_DRIVE_STRENGTH)
+		return value / MILLI;
+
+	return value;
+}
+
 static int pinctrl_scmi_map_pinconf_type_get(enum pin_config_param param,
 					     enum scmi_pinctrl_conf_type *type)
 {
@@ -166,14 +173,36 @@ static int pinctrl_scmi_map_pinconf_type_get(enum pin_config_param param,
 	return pinctrl_scmi_map_pinconf_type(param, type);
 }
 
+static bool pinctrl_scmi_pinconf_arg_ignored(enum pin_config_param param)
+{
+	switch (param) {
+	case PIN_CONFIG_BIAS_BUS_HOLD:
+	case PIN_CONFIG_BIAS_DISABLE:
+	case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
+	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
+	case PIN_CONFIG_DRIVE_OPEN_SOURCE:
+	case PIN_CONFIG_DRIVE_PUSH_PULL:
+		return true;
+	default:
+		return false;
+	}
+}
+
 static int pinctrl_scmi_map_pinconf_type_set(enum pin_config_param param,
-					     enum scmi_pinctrl_conf_type *type)
+					     enum scmi_pinctrl_conf_type *type,
+					     u32 *arg)
 {
 	if (param == PIN_CONFIG_LEVEL) {
 		*type = SCMI_PIN_OUTPUT_VALUE;
 		return 0;
 	}
 
+	if (pinctrl_scmi_pinconf_arg_ignored(param))
+		*arg = 1;
+
+	if (param == PIN_CONFIG_DRIVE_STRENGTH)
+		*arg *= MILLI;
+
 	return pinctrl_scmi_map_pinconf_type(param, type);
 }
 
@@ -203,6 +232,7 @@ static int pinctrl_scmi_pinconf_get(struct pinctrl_dev *pctldev,
 	if (ret)
 		return ret;
 
+	config_value = pinctrl_scmi_map_pinconf_arg_get(config_type, config_value);
 	*config = pinconf_to_config_packed(config_type, config_value);
 
 	return 0;
@@ -267,12 +297,13 @@ static int pinctrl_scmi_pinconf_set(struct pinctrl_dev *pctldev,
 		param = pinconf_to_config_param(configs[i]);
 		if (param == PIN_CONFIG_PERSIST_STATE)
 			continue;
-		ret = pinctrl_scmi_map_pinconf_type_set(param, &p_config_type[cnt]);
+		p_config_value[cnt] = pinconf_to_config_argument(configs[i]);
+		ret = pinctrl_scmi_map_pinconf_type_set(param, &p_config_type[cnt],
+							&p_config_value[cnt]);
 		if (ret) {
 			dev_err(pmx->dev, "Error map pinconf_type %d\n", ret);
 			goto free_config;
 		}
-		p_config_value[cnt] = pinconf_to_config_argument(configs[i]);
 		cnt++;
 	}
 
@@ -310,13 +341,13 @@ static int pinctrl_scmi_pinconf_group_set(struct pinctrl_dev *pctldev,
 
 	for (i = 0; i < num_configs; i++) {
 		param = pinconf_to_config_param(configs[i]);
-		ret = pinctrl_scmi_map_pinconf_type_set(param, &p_config_type[i]);
+		p_config_value[i] = pinconf_to_config_argument(configs[i]);
+		ret = pinctrl_scmi_map_pinconf_type_set(param, &p_config_type[i],
+							&p_config_value[i]);
 		if (ret) {
 			dev_err(pmx->dev, "Error map pinconf_type %d\n", ret);
 			goto free_config;
 		}
-
-		p_config_value[i] = pinconf_to_config_argument(configs[i]);
 	}
 
 	ret = pinctrl_ops->settings_conf(pmx->ph, group, GROUP_TYPE,
@@ -359,6 +390,7 @@ static int pinctrl_scmi_pinconf_group_get(struct pinctrl_dev *pctldev,
 	if (ret)
 		return ret;
 
+	config_value = pinctrl_scmi_map_pinconf_arg_get(config_type, config_value);
 	*config = pinconf_to_config_packed(config_type, config_value);
 
 	return 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_err`
- 外部函数：`pinconf_to_config_argument`
- 外部函数：`pinconf_to_config_packed`
- 外部函数：`pinconf_to_config_param`
- 外部函数：`settings_conf`
- 大写宏：`EINVAL`
- 大写宏：`GROUP_TYPE`
- 大写宏：`PIN_CONFIG_BIAS_BUS_HOLD`
- 大写宏：`PIN_CONFIG_BIAS_DISABLE`
- 大写宏：`PIN_CONFIG_BIAS_HIGH_IMPEDANCE`
- 大写宏：`PIN_CONFIG_BIAS_PULL_DOWN`
- 大写宏：`PIN_CONFIG_BIAS_PULL_PIN_DEFAULT`
- 大写宏：`PIN_CONFIG_BIAS_PULL_UP`
- 大写宏：`PIN_CONFIG_DRIVE_OPEN_DRAIN`
- 大写宏：`PIN_CONFIG_DRIVE_OPEN_SOURCE`
- 大写宏：`PIN_CONFIG_DRIVE_PUSH_PULL`
- 大写宏：`PIN_CONFIG_DRIVE_STRENGTH`
- 大写宏：`PIN_CONFIG_DRIVE_STRENGTH_UA`
- 大写宏：`PIN_CONFIG_INPUT_DEBOUNCE`
- 大写宏：`PIN_CONFIG_INPUT_ENABLE`
- 大写宏：`PIN_CONFIG_INPUT_SCHMITT`
- 大写宏：`PIN_CONFIG_INPUT_SCHMITT_ENABLE`
- 大写宏：`PIN_CONFIG_LEVEL`
- 大写宏：`PIN_CONFIG_MODE_LOW_POWER`
- 大写宏：`PIN_CONFIG_OUTPUT_ENABLE`
- 大写宏：`PIN_CONFIG_PERSIST_STATE`
- 大写宏：`PIN_CONFIG_POWER_SOURCE`
- 大写宏：`PIN_CONFIG_SLEW_RATE`
- 大写宏：`SCMI_PIN_BIAS_BUS_HOLD`
- 大写宏：`SCMI_PIN_BIAS_DISABLE`
- 大写宏：`SCMI_PIN_BIAS_HIGH_IMPEDANCE`
- 大写宏：`SCMI_PIN_BIAS_PULL_DEFAULT`
- 大写宏：`SCMI_PIN_BIAS_PULL_DOWN`
- 大写宏：`SCMI_PIN_BIAS_PULL_UP`
- 大写宏：`SCMI_PIN_DRIVE_OPEN_DRAIN`
- 大写宏：`SCMI_PIN_DRIVE_OPEN_SOURCE`
- 大写宏：`SCMI_PIN_DRIVE_PUSH_PULL`
- 大写宏：`SCMI_PIN_DRIVE_STRENGTH`
- 大写宏：`SCMI_PIN_INPUT_DEBOUNCE`
- 大写宏：`SCMI_PIN_INPUT_MODE`
- 大写宏：`SCMI_PIN_INPUT_SCHMITT`
- 大写宏：`SCMI_PIN_LOW_POWER_MODE`
- 大写宏：`SCMI_PIN_OEM_END`
- 大写宏：`SCMI_PIN_OEM_START`
- 大写宏：`SCMI_PIN_OUTPUT_MODE`
- 大写宏：`SCMI_PIN_OUTPUT_VALUE`
- 大写宏：`SCMI_PIN_POWER_SOURCE`
- 大写宏：`SCMI_PIN_SLEW_RATE`
- 外部类型：`Error`
- 外部类型：`pin_config_param`
- 外部类型：`scmi_pinctrl_conf_type`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a7c0cb4ae2` → 本草稿移入 `cases/defect/auto-linux-a7c0cb4ae2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
