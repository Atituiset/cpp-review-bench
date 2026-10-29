# auto-linux-3cd81ac61b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#1cb6cfab0916d18f88d44e4a23806b6c4cb355f8](https://github.com/torvalds/linux/commit/1cb6cfab0916d18f88d44e4a23806b6c4cb355f8) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 46（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1cb6cfab0916d18f88d44e4a23806b6c4cb355f8 (https://github.com/torvalds/linux/commit/1cb6cfab0916d18f88d44e4a23806b6c4cb355f8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 974；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1cb6cfab0916d18f88d44e4a23806b6c4cb355f8 Merge tag 'soundwire-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/vkoul/soundwire :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -958,8 +958,31 @@ static void sdw_modify_slave_status(struct sdw_slave *slave,
 	mutex_unlock(&bus->bus_lock);
 }
 
+static enum sdw_clk_stop_mode sdw_get_clk_stop_mode(struct sdw_slave *slave)
+{
+	enum sdw_clk_stop_mode mode;
+
+	mode = slave->prop.clk_stop_mode1 ? SDW_CLK_STOP_MODE1 : SDW_CLK_STOP_MODE0;
+
+	mutex_lock(&slave->sdw_dev_lock);
+
+	/*
+	 * Query for clock stop mode if Slave implements
+	 * ops->get_clk_stop_mode, else read from property.
+	 */
+	if (slave->probed) {
+		struct sdw_driver *drv = drv_to_sdw_driver(slave->dev.driver);
+
+		if (drv->ops && drv->ops->get_clk_stop_mode)
+			mode = drv->ops->get_clk_stop_mode(slave);
+	}
+
+	mutex_unlock(&slave->sdw_dev_lock);
+
+	return mode;
+}
+
 static int sdw_slave_clk_stop_callback(struct sdw_slave *slave,
-				       enum sdw_clk_stop_mode mode,
 				       enum sdw_clk_stop_type type)
 {
 	int ret = 0;
@@ -971,7 +994,7 @@ static int sdw_slave_clk_stop_callback(struct sdw_slave *slave,
 		struct sdw_driver *drv = drv_to_sdw_driver(dev->driver);
 
 		if (drv->ops && drv->ops->clk_stop)
-			ret = drv->ops->clk_stop(slave, mode, type);
+			ret = drv->ops->clk_stop(slave, slave->clk_stop_mode, type);
 	}
 
 	mutex_unlock(&slave->sdw_dev_lock);
@@ -980,7 +1003,6 @@ static int sdw_slave_clk_stop_callback(struct sdw_slave *slave,
 }
 
 static int sdw_slave_clk_stop_prepare(struct sdw_slave *slave,
-				      enum sdw_clk_stop_mode mode,
 				      bool prepare)
 {
 	bool wake_en;
@@ -992,7 +1014,7 @@ static int sdw_slave_clk_stop_prepare(struct sdw_slave *slave,
 	if (prepare) {
 		val = SDW_SCP_SYSTEMCTRL_CLK_STP_PREP;
 
-		if (mode == SDW_CLK_STOP_MODE1)
+		if (slave->clk_stop_mode == SDW_CLK_STOP_MODE1)
 			val |= SDW_SCP_SYSTEMCTRL_CLK_STP_MODE1;
 
 		if (wake_en)
@@ -1080,9 +1102,9 @@ int sdw_bus_prep_clk_stop(struct sdw_bus *bus)
 		/* Identify if Slave(s) are available on Bus */
 		is_slave = true;
 
-		ret = sdw_slave_clk_stop_callback(slave,
-						  SDW_CLK_STOP_MODE0,
-						  SDW_CLK_PRE_PREPARE);
+		slave->clk_stop_mode = sdw_get_clk_stop_mode(slave);
+
+		ret = sdw_slave_clk_stop_callback(slave, SDW_CLK_PRE_PREPARE);
 		if (ret < 0 && ret != -ENODATA) {
 			dev_err(&slave->dev, "clock stop pre-prepare cb failed:%d\n", ret);
 			return ret;
@@ -1092,9 +1114,7 @@ int sdw_bus_prep_clk_stop(struct sdw_bus *bus)
 		if (!slave->prop.simple_clk_stop_capable) {
 			simple_clk_stop = false;
 
-			ret = sdw_slave_clk_stop_prepare(slave,
-							 SDW_CLK_STOP_MODE0,
-							 true);
+			ret = sdw_slave_clk_stop_prepare(slave, true);
 			if (ret < 0 && ret != -ENODATA) {
 				dev_err(&slave->dev, "clock stop prepare failed:%d\n", ret);
 				return ret;
@@ -1132,9 +1152,7 @@ int sdw_bus_prep_clk_stop(struct sdw_bus *bus)
 		    slave->status != SDW_SLAVE_ALERT)
 			continue;
 
-		ret = sdw_slave_clk_stop_callback(slave,
-						  SDW_CLK_STOP_MODE0,
-						  SDW_CLK_POST_PREPARE);
+		ret = sdw_slave_clk_stop_callback(slave, SDW_CLK_POST_PREPARE);
 
 		if (ret < 0 && ret != -ENODATA) {
 			dev_err(&slave->dev, "clock stop post-prepare cb failed:%d\n", ret);
@@ -1206,18 +1224,15 @@ int sdw_bus_exit_clk_stop(struct sdw_bus *bus)
 		/* Identify if Slave(s) are available on Bus */
 		is_slave = true;
 
-		ret = sdw_slave_clk_stop_callback(slave, SDW_CLK_STOP_MODE0,
-						  SDW_CLK_PRE_DEPREPARE);
+		ret = sdw_slave_clk_stop_callback(slave, SDW_CLK_PRE_DEPREPARE);
 		if (ret < 0)
 			dev_warn(&slave->dev, "clock stop pre-deprepare cb failed:%d\n", ret);
 
 		/* Only de-prepare a Slave device if needed */
 		if (!slave->prop.simple_clk_stop_capable) {
 			simple_clk_stop = false;
 
-			ret = sdw_slave_clk_stop_prepare(slave, SDW_CLK_STOP_MODE0,
-							 false);
-
+			ret = sdw_slave_clk_stop_prepare(slave, false);
 			if (ret < 0)
 				dev_warn(&slave->dev, "clock stop deprepare failed:%d\n", ret);
 		}
@@ -1245,8 +1260,7 @@ int sdw_bus_exit_clk_stop(struct sdw_bus *bus)
 		    slave->status != SDW_SLAVE_ALERT)
 			continue;
 
-		ret = sdw_slave_clk_stop_callback(slave, S
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clk_stop`
- 外部函数：`dev_err`
- 外部函数：`dev_warn`
- 外部函数：`drv_to_sdw_driver`
- 外部函数：`mutex_unlock`
- 大写宏：`ENODATA`
- 大写宏：`SDW_CLK_POST_DEPREPARE`
- 大写宏：`SDW_CLK_POST_PREPARE`
- 大写宏：`SDW_CLK_PRE_DEPREPARE`
- 大写宏：`SDW_CLK_PRE_PREPARE`
- 大写宏：`SDW_CLK_STOP_MODE0`
- 大写宏：`SDW_CLK_STOP_MODE1`
- 大写宏：`SDW_SCP_SYSTEMCTRL_CLK_STP_MODE1`
- 大写宏：`SDW_SCP_SYSTEMCTRL_CLK_STP_PREP`
- 大写宏：`SDW_SLAVE_ALERT`
- 外部类型：`Bus`
- 外部类型：`Identify`
- 外部类型：`Only`
- 外部类型：`Slave`
- 外部类型：`sdw_clk_stop_mode`
- 外部类型：`sdw_clk_stop_type`
- 外部类型：`sdw_driver`
- 外部类型：`sdw_slave`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3cd81ac61b` → 本草稿移入 `cases/defect/auto-linux-3cd81ac61b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
