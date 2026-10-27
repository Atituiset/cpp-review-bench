# auto-linux-b6dbebbd87

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#d857e1f3faf998f2614dcdb52a90a27bc83bd71d](https://github.com/torvalds/linux/commit/d857e1f3faf998f2614dcdb52a90a27bc83bd71d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d857e1f3faf998f2614dcdb52a90a27bc83bd71d (https://github.com/torvalds/linux/commit/d857e1f3faf998f2614dcdb52a90a27bc83bd71d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 977；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d857e1f3faf998f2614dcdb52a90a27bc83bd71d Merge tag 'mfd-next-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/lee/mfd :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -959,6 +959,7 @@ static void cs42l43_boot_work(struct work_struct *work)
 {
 	struct cs42l43 *cs42l43 = container_of(work, struct cs42l43, boot_work);
 	unsigned int devid, revid, otp;
+	bool valid_id;
 	int ret;
 
 	ret = cs42l43_wait_for_attach(cs42l43);
@@ -973,19 +974,24 @@ static void cs42l43_boot_work(struct work_struct *work)
 
 	switch (devid) {
 	case CS42L43_DEVID_VAL:
+		valid_id = (cs42l43->variant_id == CS42L43_DEVID_VAL);
+		break;
 	case CS42L43B_DEVID_VAL:
-		if (devid != cs42l43->variant_id) {
-			dev_err(cs42l43->dev,
-				"Device ID (0x%06x) does not match variant ID (0x%06lx)\n",
-				devid, cs42l43->variant_id);
-			goto err;
-		}
+	case CS42L44_DEVID_VAL:
+		valid_id = (cs42l43->variant_id == CS42L43B_DEVID_VAL);
 		break;
 	default:
 		dev_err(cs42l43->dev, "Unrecognised devid: 0x%06x\n", devid);
 		goto err;
 	}
 
+	if (!valid_id) {
+		dev_err(cs42l43->dev,
+			"Device ID (0x%06x) does not match variant ID (0x%06lx)\n",
+			devid, cs42l43->variant_id);
+		goto err;
+	}
+
 	ret = regmap_read(cs42l43->regmap, CS42L43_REVID, &revid);
 	if (ret) {
 		dev_err(cs42l43->dev, "Failed to read rev: %d\n", ret);
@@ -1099,15 +1105,19 @@ static int cs42l43_power_down(struct cs42l43 *cs42l43)
 	return 0;
 }
 
-static void cs42l43_dev_remove(void *data)
+static void cs42l43_dev_power_down(void *data)
 {
 	struct cs42l43 *cs42l43 = data;
 
-	cancel_work_sync(&cs42l43->boot_work);
-
 	cs42l43_power_down(cs42l43);
 }
 
+void cs42l43_dev_remove(struct cs42l43 *cs42l43)
+{
+	flush_work(&cs42l43->boot_work);
+}
+EXPORT_SYMBOL_NS_GPL(cs42l43_dev_remove, "MFD_CS42L43");
+
 int cs42l43_dev_probe(struct cs42l43 *cs42l43)
 {
 	int i, ret;
@@ -1153,7 +1163,7 @@ int cs42l43_dev_probe(struct cs42l43 *cs42l43)
 	if (ret)
 		return ret;
 
-	ret = devm_add_action_or_reset(cs42l43->dev, cs42l43_dev_remove, cs42l43);
+	ret = devm_add_action_or_reset(cs42l43->dev, cs42l43_dev_power_down, cs42l43);
 	if (ret)
 		return ret;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cancel_work_sync`
- 外部函数：`container_of`
- 外部函数：`dev_err`
- 外部函数：`devm_add_action_or_reset`
- 外部函数：`gpiod_set_raw_value_cansleep`
- 外部函数：`regcache_cache_only`
- 外部函数：`regmap_read`
- 外部函数：`regmap_write`
- 外部函数：`regulator_bulk_disable`
- 外部函数：`regulator_disable`
- 外部函数：`sdw_slave_wait_for_init`
- 大写宏：`CS42L43B_DEVID_VAL`
- 大写宏：`CS42L43_DEVID_VAL`
- 大写宏：`CS42L43_N_SUPPLIES`
- 大写宏：`CS42L43_OSC_DIV2_EN_MASK`
- 大写宏：`CS42L43_OSC_DIV_SEL`
- 大写宏：`CS42L43_REVID`
- 大写宏：`OSC_DIV`
- 外部类型：`Device`
- 外部类型：`Failed`
- 外部类型：`SoundWire`
- 外部类型：`The`
- 外部类型：`Unrecognised`
- 外部类型：`cs42l43`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b6dbebbd87` → 本草稿移入 `cases/defect/auto-linux-b6dbebbd87/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
