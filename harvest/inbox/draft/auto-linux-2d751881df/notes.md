# auto-linux-2d751881df

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7641bffc3721f0d1650e52935369ec7f7bf84126](https://github.com/torvalds/linux/commit/7641bffc3721f0d1650e52935369ec7f7bf84126) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7641bffc3721f0d1650e52935369ec7f7bf84126 (https://github.com/torvalds/linux/commit/7641bffc3721f0d1650e52935369ec7f7bf84126)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1259；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7641bffc3721f0d1650e52935369ec7f7bf84126 Merge tag 'usb-7.4-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/usb :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1256,11 +1256,12 @@ static void uvcg_extension_drop(struct config_group *group, struct config_item *
 
 	mutex_lock(&opts->lock);
 
-	config_item_put(item);
 	list_del(&xu->list);
 	kfree(xu->desc.baSourceID);
 	kfree(xu->desc.bmControls);
 
+	config_item_put(item);
+
 	mutex_unlock(&opts->lock);
 }
 
@@ -2142,14 +2143,26 @@ static struct configfs_attribute *uvcg_frame_attrs2[] = {
 	NULL,
 };
 
+static void uvcg_frame_release(struct config_item *item)
+{
+	struct uvcg_frame *frm = to_uvcg_frame(item);
+
+	kfree(frm->dw_frame_interval);
+	kfree(frm);
+}
+
+static const struct configfs_item_operations uvcg_frame_item_ops = {
+	.release	= uvcg_frame_release,
+};
+
 static const struct config_item_type uvcg_frame_type1 = {
-	.ct_item_ops	= &uvcg_config_item_ops,
+	.ct_item_ops	= &uvcg_frame_item_ops,
 	.ct_attrs	= uvcg_frame_attrs1,
 	.ct_owner	= THIS_MODULE,
 };
 
 static const struct config_item_type uvcg_frame_type2 = {
-	.ct_item_ops    = &uvcg_config_item_ops,
+	.ct_item_ops    = &uvcg_frame_item_ops,
 	.ct_attrs       = uvcg_frame_attrs2,
 	.ct_owner       = THIS_MODULE,
 };
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`config_item_put`
- 外部函数：`kfree`
- 外部函数：`list_del`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 大写宏：`NULL`
- 大写宏：`THIS_MODULE`
- 外部类型：`config_item_type`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-2d751881df` → 本草稿移入 `cases/defect/auto-linux-2d751881df/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
