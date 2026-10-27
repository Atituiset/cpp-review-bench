# auto-linux-7c689374a4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#5b93dc1c0c61fd0fe575cb473e9e25f929fda509](https://github.com/torvalds/linux/commit/5b93dc1c0c61fd0fe575cb473e9e25f929fda509) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 54 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5b93dc1c0c61fd0fe575cb473e9e25f929fda509 (https://github.com/torvalds/linux/commit/5b93dc1c0c61fd0fe575cb473e9e25f929fda509)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 36；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5b93dc1c0c61fd0fe575cb473e9e25f929fda509 Merge tag 'leds-next-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/lee/leds :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7,9 +7,13 @@
  * Author: Richard Purdie <rpurdie@openedhand.com>
  */
 
+#include <linux/bug.h>
+#include <linux/cleanup.h>
+#include <linux/compiler.h>
 #include <linux/export.h>
 #include <linux/kernel.h>
 #include <linux/list.h>
+#include <linux/lockdep.h>
 #include <linux/spinlock.h>
 #include <linux/device.h>
 #include <linux/timer.h>
@@ -33,9 +37,38 @@ trigger_relevant(struct led_classdev *led_cdev, struct led_trigger *trig)
 	return !trig->trigger_type || trig->trigger_type == led_cdev->trigger_type;
 }
 
-ssize_t led_trigger_write(struct file *filp, struct kobject *kobj,
-			  const struct bin_attribute *bin_attr, char *buf,
-			  loff_t pos, size_t count)
+static bool __led_trigger_is_hw_controlled(struct led_classdev *led_cdev)
+{
+	lockdep_assert_held(&led_cdev->trigger_lock);
+
+	if (!led_cdev->trigger)
+		return false;
+
+	if (!led_cdev->hw_control_trigger ||
+	    strcmp(led_cdev->hw_control_trigger, led_cdev->trigger->name))
+		return false;
+
+	if (led_cdev->trigger->hw_offloaded)
+		return led_cdev->trigger->hw_offloaded(led_cdev);
+
+	dev_warn_once(led_cdev->dev,
+		      "Hardware control trigger %s doesn't provide offloaded state\n",
+		      led_cdev->trigger->name);
+
+	/* Otherwise assume private triggers are always offloaded. */
+	return led_cdev->trigger->trigger_type;
+}
+
+bool led_trigger_is_hw_controlled(struct led_classdev *led_cdev)
+{
+	guard(rwsem_read)(&led_cdev->trigger_lock);
+	return __led_trigger_is_hw_controlled(led_cdev);
+}
+EXPORT_SYMBOL_GPL(led_trigger_is_hw_controlled);
+
+static ssize_t trigger_write(struct file *filp, struct kobject *kobj,
+			     const struct bin_attribute *bin_attr, char *buf,
+			     loff_t pos, size_t count)
 {
 	struct device *dev = kobj_to_dev(kobj);
 	struct led_classdev *led_cdev = dev_get_drvdata(dev);
@@ -78,7 +111,6 @@ ssize_t led_trigger_write(struct file *filp, struct kobject *kobj,
 	mutex_unlock(&led_cdev->led_access);
 	return ret;
 }
-EXPORT_SYMBOL_GPL(led_trigger_write);
 
 __printf(3, 4)
 static int led_trigger_snprintf(char *buf, ssize_t size, const char *fmt, ...)
@@ -130,9 +162,9 @@ static int led_trigger_format(char *buf, size_t size,
  * attribute, which is not limited by length. This is _not_ good design, do not
  * copy it.
  */
-ssize_t led_trigger_read(struct file *filp, struct kobject *kobj,
-			const struct bin_attribute *attr, char *buf,
-			loff_t pos, size_t count)
+static ssize_t trigger_read(struct file *filp, struct kobject *kobj,
+			    const struct bin_attribute *attr, char *buf,
+			    loff_t pos, size_t count)
 {
 	struct device *dev = kobj_to_dev(kobj);
 	struct led_classdev *led_cdev = dev_get_drvdata(dev);
@@ -160,10 +192,53 @@ ssize_t led_trigger_read(struct file *filp, struct kobject *kobj,
 
 	return len;
 }
-EXPORT_SYMBOL_GPL(led_trigger_read);
+static const BIN_ATTR_RW(trigger, 0);
+
+static const struct bin_attribute *const led_trigger_bin_attrs[] = {
+	&bin_attr_trigger,
+	NULL
+};
+
+static ssize_t trigger_may_offload_to_hw_show(struct device *dev,
+					      const struct device_attribute *attr, char *buf)
+{
+	struct led_classdev *led_cdev = dev_get_drvdata(dev);
+	bool offloaded = led_trigger_is_hw_controlled(led_cdev);
+
+	return sysfs_emit(buf, "%s%s%s\n",
+			  offloaded ? "[" : "",
+			  led_cdev->hw_control_trigger,
+			  offloaded ? "]" : "");
+}
+static const DEVICE_ATTR_RO(trigger_may_offload_to_hw);
+
+static const struct attribute *const led_trigger_attrs[] = {
+	&dev_attr_trigger_may_offload_to_hw.attr,
+	NULL
+};
+
+static umode_t led_trigger_is_visible(struct kobject *kobj,
+				      const struct attribute *attr, int idx)
+{
+	struct device *dev = kobj_to_dev(kobj);
+	struct led_classdev *led_cdev = dev_get_drvdata(dev);
+
+	if (attr == &dev_attr_trigger_may_offload_to_hw.attr)
+		return led_cdev->hw_control_trigger ? attr->mode : 0;
+
+	return attr->mode;
+}
+
+const struct attribute_group led_trigger_group = {
+	.bin_attrs = led_trigger_bin_attrs,
+	.attrs_const = led_trigger_attrs,
+	.is
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_get_drvdata`
- 外部函数：`devres_add`
- 外部函数：`devres_alloc`
- 外部函数：`devres_free`
- 外部函数：`down_read`
- 外部函数：`down_write`
- 外部函数：`kobj_to_dev`
- 外部函数：`kvfree`
- 外部函数：`kvmalloc`
- 外部函数：`led_set_brightness`
- 外部函数：`led_trigger_snprintf`
- 外部函数：`list_add_tail`
- 外部函数：`list_del_init`
- 外部函数：`list_empty_careful`
- 外部函数：`memory_read_from_buffer`
- 外部函数：`mutex_unlock`
- 外部函数：`spin_lock`
- 外部函数：`spin_lock_init`
- 外部函数：`strcmp`
- 外部函数：`up_read`
- 外部函数：`up_write`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vscnprintf`
- 外部函数：`vsnprintf`
- 大写宏：`EEXIST`
- 大写宏：`ENOMEM`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`GFP_KERNEL`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`LED`
- 大写宏：`LED_INIT_DEFAULT_TRIGGER`
- 大写宏：`LED_OFF`
- 大写宏：`NULL`
- 外部类型：`Add`
- 外部类型：`Author`
- 外部类型：`Caller`
- 外部类型：`Interface`
- 外部类型：`LEDs`
- 外部类型：`Make`
- 外部类型：`Purdie`
- 外部类型：`Register`
- 外部类型：`Remove`
- 外部类型：`Richard`
- 外部类型：`Simple`
- 外部类型：`This`
- 外部类型：`Trigger`
- 外部类型：`bin_attribute`
- 外部类型：`device`
- 外部类型：`file`
- 外部类型：`kobject`
- 外部类型：`led_brightness`
- 外部类型：`led_classdev`
- 外部类型：`led_trigger`
- 外部类型：`loff_t`
- 外部类型：`size_t`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7c689374a4` → 本草稿移入 `cases/defect/auto-linux-7c689374a4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
