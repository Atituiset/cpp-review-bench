# auto-linux-e52641ba54

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4da9dedf3fc59137fd86d664693776aa38cb97f5](https://github.com/torvalds/linux/commit/4da9dedf3fc59137fd86d664693776aa38cb97f5) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4da9dedf3fc59137fd86d664693776aa38cb97f5 (https://github.com/torvalds/linux/commit/4da9dedf3fc59137fd86d664693776aa38cb97f5)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4da9dedf3fc59137fd86d664693776aa38cb97f5 Merge tag 'iommu-updates-v7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/iommu/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -33,6 +33,7 @@
 #include <linux/console.h>
 #include <linux/slab.h>
 #include <linux/iommu.h>
+#include <linux/amd-iommu.h>
 #include <linux/pci.h>
 #include <linux/pci-p2pdma.h>
 #include <linux/apple-gmux.h>
@@ -3751,6 +3752,28 @@ amdgpu_device_should_register_switcheroo(struct amdgpu_device *adev, bool px)
 		       apple_gmux_detect(NULL, NULL)));
 }
 
+static inline bool amdgpu_device_identity(struct amdgpu_device *adev)
+{
+	struct pci_dev *pdev = adev->pdev;
+	struct iommu_domain *domain = iommu_get_domain_for_dev(&pdev->dev);
+
+	if (!domain)
+		return false;
+
+	return domain->type == IOMMU_DOMAIN_IDENTITY;
+}
+
+static bool amdgpu_device_use_perfopt(struct amdgpu_device *adev)
+{
+	if (amdgpu_iommu_perfopt == 0)
+		return false;
+
+	if (!(adev->flags & AMD_IS_APU))
+		return false;
+
+	return amdgpu_device_identity(adev);
+}
+
 /**
  * amdgpu_device_init - initialize the driver
  *
@@ -3958,6 +3981,16 @@ int amdgpu_device_init(struct amdgpu_device *adev,
 	if (r)
 		return r;
 
+	if (amdgpu_device_use_perfopt(adev)) {
+		int perfopt_ret = amd_iommu_enable_perfopt(pdev);
+
+		/* Optional optimization; a failure to arm it must not abort probe. */
+		if (perfopt_ret)
+			dev_warn(adev->dev,
+				 "Failed to enable IOMMU PerfOpt (%d); continuing without it\n",
+				 perfopt_ret);
+	}
+
 	/*
 	 * No need to remove conflicting FBs for non-display class devices.
 	 * This prevents the sysfb from being freed accidently.
@@ -4327,6 +4360,9 @@ void amdgpu_device_fini_hw(struct amdgpu_device *adev)
 
 	amdgpu_gart_dummy_page_fini(adev);
 
+	if (amdgpu_device_use_perfopt(adev))
+		amd_iommu_disable_perfopt(adev->pdev);
+
 	if (pci_dev_is_disconnected(adev->pdev))
 		amdgpu_device_unmap_mmio(adev);
 
@@ -4690,6 +4726,20 @@ int amdgpu_device_resume(struct drm_device *dev, bool notify_clients)
 	if (dev->switch_power_state == DRM_SWITCH_POWER_OFF)
 		return 0;
 
+	if (amdgpu_device_use_perfopt(adev)) {
+		int perfopt_ret = amd_iommu_enable_perfopt(adev->pdev);
+
+		/*
+		 * Must not return on failure: a bare return would leak the
+		 * SR-IOV VF exclusive-mode acquisition taken above (released
+		 * via the exit: path).
+		 */
+		if (perfopt_ret)
+			dev_warn(adev->dev,
+				 "Failed to enable IOMMU PerfOpt (%d); continuing without it\n",
+				 perfopt_ret);
+	}
+
 	if (adev->in_s0ix)
 		amdgpu_dpm_gfx_state_change(adev, sGpuChangeState_D0Entry);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`amdgpu_doorbell_fini`
- 外部函数：`amdgpu_dpm_gfx_state_change`
- 外部函数：`amdgpu_gart_dummy_page_fini`
- 外部函数：`apple_gmux_detect`
- 外部函数：`arch_io_free_memtype_wc`
- 外部函数：`arch_phys_wc_del`
- 外部函数：`iounmap`
- 外部函数：`pci_dev_is_disconnected`
- 外部函数：`unmap_mapping_range`
- 大写宏：`CPU`
- 大写宏：`DRM_SWITCH_POWER_OFF`
- 大写宏：`NULL`
- 大写宏：`VRAM`
- 外部类型：`Clear`
- 外部类型：`Doorbell`
- 外部类型：`FBs`
- 外部类型：`Memory`
- 外部类型：`No`
- 外部类型：`This`
- 外部类型：`Unmap`
- 外部类型：`amdgpu_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e52641ba54` → 本草稿移入 `cases/defect/auto-linux-e52641ba54/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
