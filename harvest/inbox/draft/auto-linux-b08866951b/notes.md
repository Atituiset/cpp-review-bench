# auto-linux-b08866951b

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
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1cb6cfab0916d18f88d44e4a23806b6c4cb355f8 (https://github.com/torvalds/linux/commit/1cb6cfab0916d18f88d44e4a23806b6c4cb355f8)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 113；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1cb6cfab0916d18f88d44e4a23806b6c4cb355f8 Merge tag 'soundwire-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/vkoul/soundwire :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -71,6 +71,13 @@ int sdw_slave_uevent(const struct device *dev, struct kobj_uevent_env *env)
 	return 0;
 }
 
+static void sdw_slave_ida_free(void *data)
+{
+	struct sdw_slave *slave = data;
+
+	ida_free(&slave->bus->slave_ida, slave->index);
+}
+
 static int sdw_bus_probe(struct device *dev)
 {
 	struct sdw_slave *slave = dev_to_sdw_dev(dev);
@@ -104,15 +111,16 @@ static int sdw_bus_probe(struct device *dev)
 		return ret;
 	}
 	slave->index = ret;
+	ret = devm_add_action_or_reset(dev, sdw_slave_ida_free, slave);
+	if (ret)
+		return ret;
 
 	/* Create IRQ mapping now so the driver can get it in probe() */
 	sdw_irq_create_mapping(slave);
 
 	ret = drv->probe(slave, id);
-	if (ret) {
-		ida_free(&slave->bus->slave_ida, slave->index);
+	if (ret)
 		return ret;
-	}
 
 	mutex_lock(&slave->sdw_dev_lock);
 
@@ -170,8 +178,6 @@ static void sdw_bus_remove(struct device *dev)
 
 	if (drv->remove)
 		drv->remove(slave);
-
-	ida_free(&slave->bus->slave_ida, slave->index);
 }
 
 static void sdw_bus_shutdown(struct device *dev)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_to_sdw_dev`
- 外部函数：`drv_to_sdw_driver`
- 外部函数：`ida_free`
- 外部函数：`mutex_lock`
- 外部函数：`probe`
- 外部函数：`remove`
- 外部函数：`sdw_irq_create_mapping`
- 外部函数：`shutdown`
- 大写宏：`IRQ`
- 外部类型：`Create`
- 外部类型：`device`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-b08866951b` → 本草稿移入 `cases/defect/auto-linux-b08866951b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
