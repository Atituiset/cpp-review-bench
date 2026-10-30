# auto-linux-0249a91399

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4dd16e31135ea8437a720222df34e8745f385d6d](https://github.com/torvalds/linux/commit/4dd16e31135ea8437a720222df34e8745f385d6d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4dd16e31135ea8437a720222df34e8745f385d6d (https://github.com/torvalds/linux/commit/4dd16e31135ea8437a720222df34e8745f385d6d)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 69；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4dd16e31135ea8437a720222df34e8745f385d6d Merge tag 'net-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/netdev/net :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6,6 +6,7 @@
 #include <linux/pci.h>
 #include <linux/slab.h>
 #include <linux/spinlock.h>
+#include <linux/srcu.h>
 #include <linux/xarray.h>
 #include "ice_adapter.h"
 #include "ice.h"
@@ -54,30 +55,38 @@ static unsigned long ice_adapter_xa_index(struct pci_dev *pdev)
 static struct ice_adapter *ice_adapter_new(struct pci_dev *pdev)
 {
 	struct ice_adapter *adapter;
+	int err;
 
 	adapter = kzalloc_obj(*adapter);
 	if (!adapter)
 		return NULL;
 
+	err = init_srcu_struct(&adapter->ports.srcu);
+	if (err) {
+		kfree(adapter);
+		return NULL;
+	}
+
 	adapter->index = ice_adapter_index(pdev);
 	spin_lock_init(&adapter->ptp_gltsyn_time_lock);
 	spin_lock_init(&adapter->txq_ctx_lock);
 	for (int i = 0; i < ARRAY_SIZE(adapter->cpi_phy_lock); i++)
 		mutex_init(&adapter->cpi_phy_lock[i]);
 	refcount_set(&adapter->refcount, 1);
 
-	mutex_init(&adapter->ports.lock);
-	INIT_LIST_HEAD(&adapter->ports.ports);
+	spin_lock_init(&adapter->ports.lock);
+	INIT_LIST_HEAD(&adapter->ports.list);
 
 	return adapter;
 }
 
 static void ice_adapter_free(struct ice_adapter *adapter)
 {
-	WARN_ON(!list_empty(&adapter->ports.ports));
+	WARN_ON(!list_empty(&adapter->ports.list));
 	for (int i = 0; i < ARRAY_SIZE(adapter->cpi_phy_lock); i++)
 		mutex_destroy(&adapter->cpi_phy_lock[i]);
-	mutex_destroy(&adapter->ports.lock);
+
+	cleanup_srcu_struct(&adapter->ports.srcu);
 
 	kfree(adapter);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_empty`
- 外部函数：`mutex_destroy`
- 外部函数：`mutex_init`
- 外部函数：`pci_get_dsn`
- 外部函数：`refcount_set`
- 外部函数：`spin_lock_init`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`BIT_ULL`
- 大写宏：`E825C`
- 大写宏：`ICE_DEV_ID_E825C_BACKPLANE`
- 大写宏：`ICE_DEV_ID_E825C_QSFP`
- 大写宏：`ICE_DEV_ID_E825C_SFP`
- 大写宏：`ICE_DEV_ID_E825C_SGMII`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`NAC`
- 大写宏：`NULL`
- 大写宏：`NVM`
- 大写宏：`WARN_ON`
- 外部类型：`Device`
- 外部类型：`Instead`
- 外部类型：`NACs`
- 外部类型：`Number`
- 外部类型：`Serial`
- 外部类型：`This`
- 外部类型：`We`
- 外部类型：`ice_adapter`
- 外部类型：`pci_dev`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0249a91399` → 本草稿移入 `cases/defect/auto-linux-0249a91399/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
