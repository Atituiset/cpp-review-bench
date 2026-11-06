# auto-linux-bcbf36ff2d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#3e516eae690b3f24c19168cc0fe605e1c610eb21](https://github.com/torvalds/linux/commit/3e516eae690b3f24c19168cc0fe605e1c610eb21) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 38 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3e516eae690b3f24c19168cc0fe605e1c610eb21 (https://github.com/torvalds/linux/commit/3e516eae690b3f24c19168cc0fe605e1c610eb21)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 29（原始 PR diff 行 1557；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3e516eae690b3f24c19168cc0fe605e1c610eb21 Merge tag 'hwmon-for-v7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -19,6 +19,7 @@
 #include <linux/hwmon.h>
 #include <linux/jiffies.h>
 #include <linux/ktime.h>
+#include <linux/limits.h>
 #include <linux/module.h>
 #include <linux/seq_file.h>
 #include <linux/unaligned.h>
@@ -132,6 +133,7 @@ static u8 aquaero_secondary_ctrl_report[] = {
 #define AQUAERO_FAN_CURRENT_OFFSET		0x06
 #define AQUAERO_FAN_POWER_OFFSET		0x08
 #define AQUAERO_FAN_SPEED_OFFSET		0x00
+#define AQUAERO_MIN_STATUS_REPORT_SIZE		(0x18B + AQUAERO_FAN_POWER_OFFSET + sizeof(u16))
 static u16 aquaero_sensor_fan_offsets[] = { 0x167, 0x173, 0x17f, 0x18B };
 
 /* Control report offsets for the Aquaero fan controllers */
@@ -155,6 +157,8 @@ static u16 aquaero_ctrl_fan_offsets[] = { 0x20c, 0x220, 0x234, 0x248 };
 #define D5NEXT_5V_VOLTAGE		0x39
 #define D5NEXT_12V_VOLTAGE		0x37
 #define D5NEXT_VIRTUAL_SENSORS_START	0x3f
+#define D5NEXT_MIN_STATUS_REPORT_SIZE	(D5NEXT_PUMP_OFFSET + AQC_FAN_SPEED_OFFSET + \
+					 sizeof(u16))
 static u16 d5next_sensor_fan_offsets[] = { D5NEXT_PUMP_OFFSET, D5NEXT_FAN_OFFSET };
 
 /* Control report offsets for the D5 Next pump */
@@ -179,11 +183,14 @@ static u16 d5next_ctrl_fan_offsets[] = { 0x97, 0x42 };	/* Pump and fan speed (fr
 #define AQUASTREAMULT_FAN_CURRENT_OFFSET	0x00
 #define AQUASTREAMULT_FAN_POWER_OFFSET		0x04
 #define AQUASTREAMULT_FAN_SPEED_OFFSET		0x06
+#define AQUASTREAMULT_MIN_STATUS_REPORT_SIZE	(AQUASTREAMULT_PRESSURE_OFFSET + sizeof(u16))
 static u16 aquastreamult_sensor_fan_offsets[] = { AQUASTREAMULT_FAN_OFFSET };
 
 /* Spec and sensor report offset for the Farbwerk RGB controller */
 #define FARBWERK_NUM_SENSORS		4
 #define FARBWERK_SENSOR_START		0x2f
+#define FARBWERK_MIN_STATUS_REPORT_SIZE	(FARBWERK_SENSOR_START + \
+					 FARBWERK_NUM_SENSORS * AQC_SENSOR_SIZE)
 
 /* Specs of the Farbwerk 360 RGB controller */
 #define FARBWERK360_NUM_SENSORS			4
@@ -193,6 +200,8 @@ static u16 aquastreamult_sensor_fan_offsets[] = { AQUASTREAMULT_FAN_OFFSET };
 /* Sensor report offsets for the Farbwerk 360 */
 #define FARBWERK360_SENSOR_START		0x32
 #define FARBWERK360_VIRTUAL_SENSORS_START	0x3a
+#define FARBWERK360_MIN_STATUS_REPORT_SIZE	(FARBWERK360_VIRTUAL_SENSORS_START + \
+					 FARBWERK360_NUM_VIRTUAL_SENSORS * AQC_SENSOR_SIZE)
 
 /* Control report offsets for the Farbwerk 360 */
 #define FARBWERK360_TEMP_CTRL_OFFSET		0x8
@@ -209,6 +218,7 @@ static u16 aquastreamult_sensor_fan_offsets[] = { AQUASTREAMULT_FAN_OFFSET };
 #define OCTO_SENSOR_START		0x3D
 #define OCTO_VIRTUAL_SENSORS_START	0x45
 #define OCTO_FLOW_SENSOR_OFFSET		0x7B
+#define OCTO_MIN_STATUS_REPORT_SIZE	(0xD8 + AQC_FAN_SPEED_OFFSET + sizeof(u16))
 static u16 octo_sensor_fan_offsets[] = { 0x7D, 0x8A, 0x97, 0xA4, 0xB1, 0xBE, 0xCB, 0xD8 };
 
 /* Control report offsets for the Octo */
@@ -229,6 +239,7 @@ static u16 octo_ctrl_fan_offsets[] = { 0x5B, 0xB0, 0x105, 0x15A, 0x1AF, 0x204, 0
 #define QUADRO_SENSOR_START		0x34
 #define QUADRO_VIRTUAL_SENSORS_START	0x3c
 #define QUADRO_FLOW_SENSOR_OFFSET	0x6e
+#define QUADRO_MIN_STATUS_REPORT_SIZE	(0x97 + AQC_FAN_SPEED_OFFSET + sizeof(u16))
 static u16 quadro_sensor_fan_offsets[] = { 0x70, 0x7D, 0x8A, 0x97 };
 
 /* Control report offsets for the Quadro */
@@ -248,6 +259,7 @@ static u16 quadro_ctrl_fan_offsets[] = { 0x37, 0x8c, 0xe1, 0x136 }; /* Fan speed
 #define HIGHFLOWNEXT_CONDUCTIVITY	95
 #define HIGHFLOWNEXT_5V_VOLTAGE		97
 #define HIGHFLOWNEXT_5V_VOLTAGE_USB	99
+#define HIGHFLOWNEXT_MIN_STATUS_REPORT_SIZE	(HIGHFLOWNEXT_5V_VOLTAGE_USB + sizeof(u16))
 
 /* Specs of the Leakshield */
 #define LEAKSHIELD_NUM_SENSORS		2
@@ -263,6 +275,7 @@ static u16 quadro_ctrl_fan_offsets[] = { 0x37, 0x8c, 0xe1, 0x136 }; /* Fan speed
 #define LEAKSHIELD_FLOW_IN		111
 #define LEAKSHIELD_RESERVOIR_VOLUME	313
 #define LEAKSHIELD_RESERVOIR_FILLED	311
+#define LEAKSHIELD_MIN_STATUS_REPORT_SIZE	(LEAKSHIELD_RESERVOIR_VOLUME + sizeof(u16))
 
 /* Specs of the Aquastream XT pump */
 #define AQUASTREAMXT_SERIAL_START		0x3a
@@ -554,6 +567,7 @@ struct aqc_data {
 	const char *name;
 
 	int status_report_id;	/* Used
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_get_drvdata`
- 外部函数：`devm_kzalloc`
- 外部函数：`get_unaligned_be16`
- 外部函数：`get_unaligned_le16`
- 外部函数：`hid_get_drvdata`
- 大写宏：`DIV_ROUND_CLOSEST`
- 大写宏：`ENODATA`
- 大写宏：`ENODEV`
- 大写宏：`GFP_KERNEL`
- 大写宏：`RGB`
- 外部类型：`Aquaero`
- 外部类型：`Aquastream`
- 外部类型：`Calculation`
- 外部类型：`Control`
- 外部类型：`Delay`
- 外部类型：`Farbwerk`
- 外部类型：`For`
- 外部类型：`General`
- 外部类型：`How`
- 外部类型：`If`
- 外部类型：`Info`
- 外部类型：`Label`
- 外部类型：`Leakshield`
- 外部类型：`Max`
- 外部类型：`Next`
- 外部类型：`Octo`
- 外部类型：`Quadro`
- 外部类型：`Sensor`
- 外部类型：`Spec`
- 外部类型：`Specs`
- 外部类型：`The`
- 外部类型：`Used`
- 外部类型：`dentry`
- 外部类型：`device`
- 外部类型：`hid_device`
- 外部类型：`hid_device_id`
- 外部类型：`hwmon_sensor_types`
- 外部类型：`ktime_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-bcbf36ff2d` → 本草稿移入 `cases/defect/auto-linux-bcbf36ff2d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
