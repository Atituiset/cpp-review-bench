# auto-linux-7d3f25f817

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ebb7a8dc1e74c705daffe11c2faf38ec2628f838](https://github.com/torvalds/linux/commit/ebb7a8dc1e74c705daffe11c2faf38ec2628f838) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ebb7a8dc1e74c705daffe11c2faf38ec2628f838 (https://github.com/torvalds/linux/commit/ebb7a8dc1e74c705daffe11c2faf38ec2628f838)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1305；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ebb7a8dc1e74c705daffe11c2faf38ec2628f838 Merge tag 'soc-fixes-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/soc/soc :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1223,6 +1223,7 @@ struct notifier_cb_info {
 	ffa_fwk_notifier_cb fwk_cb;
 	ffa_notifier_cb cb;
 	void *cb_data;
+	u16 notify_id;
 };
 
 static int
@@ -1302,7 +1303,8 @@ notifier_hnode_get_by_vmid(u16 notify_id, int vmid)
 	struct notifier_cb_info *node;
 
 	hash_for_each_possible(drv_info->notifier_hash, node, hnode, notify_id)
-		if (node->fwk_cb && vmid == node->dev->vm_id)
+		if (node->notify_id == notify_id && node->fwk_cb &&
+		    vmid == node->dev->vm_id)
 			return node;
 
 	return NULL;
@@ -1317,7 +1319,8 @@ notifier_hnode_get_by_vmid_uuid(u16 notify_id, int vmid, const uuid_t *uuid)
 		return notifier_hnode_get_by_vmid(notify_id, vmid);
 
 	hash_for_each_possible(drv_info->notifier_hash, node, hnode, notify_id)
-		if (node->fwk_cb && vmid == node->dev->vm_id &&
+		if (node->notify_id == notify_id && node->fwk_cb &&
+		    vmid == node->dev->vm_id &&
 		    uuid_equal(&node->dev->uuid, uuid))
 			return node;
 
@@ -1330,7 +1333,8 @@ notifier_hnode_get_by_type(u16 notify_id, enum notify_type type)
 	struct notifier_cb_info *node;
 
 	hash_for_each_possible(drv_info->notifier_hash, node, hnode, notify_id)
-		if (node->cb && type == ffa_notify_type_get(node->dev->vm_id))
+		if (node->notify_id == notify_id && node->cb &&
+		    type == ffa_notify_type_get(node->dev->vm_id))
 			return node;
 
 	return NULL;
@@ -1422,6 +1426,7 @@ static int __ffa_notify_request(struct ffa_device *dev, bool is_per_vcpu,
 
 	cb_info->dev = dev;
 	cb_info->cb_data = cb_data;
+	cb_info->notify_id = notify_id;
 	if (is_framework)
 		cb_info->fwk_cb = cb;
 	else
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`hash_for_each_possible`
- 外部函数：`uuid_equal`
- 大写宏：`BIT`
- 大写宏：`NON_SECURE_VM`
- 大写宏：`NS_HYP_FRAMEWORK`
- 大写宏：`NULL`
- 大写宏：`SECURE_PARTITION`
- 大写宏：`SPM_FRAMEWORK`
- 外部类型：`ffa_device`
- 外部类型：`hlist_node`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7d3f25f817` → 本草稿移入 `cases/defect/auto-linux-7d3f25f817/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
