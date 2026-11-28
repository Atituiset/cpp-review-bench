# auto-linux-fc288d6a70

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c639ac6b9cf100b860408a3cf8d7eab268866cd7](https://github.com/torvalds/linux/commit/c639ac6b9cf100b860408a3cf8d7eab268866cd7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 159 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c639ac6b9cf100b860408a3cf8d7eab268866cd7 (https://github.com/torvalds/linux/commit/c639ac6b9cf100b860408a3cf8d7eab268866cd7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c639ac6b9cf100b860408a3cf8d7eab268866cd7 Merge tag 'drm-fixes-2026-11-28' of https://gitlab.freedesktop.org/drm/kernel :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1050,6 +1050,8 @@ int amdgpu_do_asic_reset(struct list_head *device_list_handle,
 
 int amdgpu_device_reinit_after_reset(struct amdgpu_reset_context *reset_context);
 
+bool amdgpu_device_is_removable(struct amdgpu_device *adev);
+
 int emu_soc_asic_init(struct amdgpu_device *adev);
 
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`amd_reset_method`
- 外部函数：`bool`
- 外部函数：`int`
- 外部函数：`region`
- 外部函数：`ssize_t`
- 外部函数：`time`
- 外部函数：`u32`
- 外部函数：`u64`
- 外部函数：`uint64_t`
- 外部函数：`void`
- 大写宏：`AMDGPU_ENFORCE_ISOLATION_DISABLE`
- 大写宏：`AMDGPU_ENFORCE_ISOLATION_ENABLE`
- 大写宏：`AMDGPU_ENFORCE_ISOLATION_ENABLE_LEGACY`
- 大写宏：`AMDGPU_ENFORCE_ISOLATION_NO_CLEANER_SHADER`
- 大写宏：`AMDGPU_INIT_LEVEL_DEFAULT`
- 大写宏：`AMDGPU_INIT_LEVEL_MINIMAL_XGMI`
- 大写宏：`AMDGPU_INIT_LEVEL_RESET_RECOVERY`
- 大写宏：`AMDGPU_MAX_I2C_BUS`
- 大写宏：`AMDGPU_UID_TYPE_MAX`
- 大写宏：`ASIC`
- 大写宏：`BACO`
- 大写宏：`BIOS`
- 大写宏：`CONFIG_DRM_AMD_ACP`
- 大写宏：`DATA`
- 大写宏：`GRBM`
- 大写宏：`MM_INDEX`
- 大写宏：`UID`
- 外部类型：`Indirect`
- 外部类型：`Khz`
- 外部类型：`PCIe`
- 外部类型：`Protects`
- 外部类型：`Register`
- 外部类型：`The`
- 外部类型：`amd_asic_type`
- 外部类型：`amd_pm_funcs`
- 外部类型：`amd_reset_method`
- 外部类型：`amdgpu_acp`
- 外部类型：`amdgpu_bo`
- 外部类型：`amdgpu_doorbell`
- 外部类型：`amdgpu_hive_info`
- 外部类型：`amdgpu_i2c_chan`
- 外部类型：`amdgpu_pll`
- 外部类型：`amdgpu_reg_access`
- 外部类型：`amdgpu_reg_state`
- 外部类型：`amdgpu_ring`
- 外部类型：`amdgpu_video_codecs`
- 外部类型：`amdgpu_xcp_mgr`
- 外部类型：`debugfs_blob_wrapper`
- 外部类型：`dev_pm_domain`
- 外部类型：`device`
- 外部类型：`drm_device`
- 外部类型：`mutex`
- 外部类型：`notifier_block`
- 外部类型：`pci_dev`
- 外部类型：`pci_saved_state`
- 外部类型：`resource_size_t`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`
- 外部类型：`uint32_t`
- 外部类型：`uint64_t`
- 外部类型：`uint8_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-fc288d6a70` → 本草稿移入 `cases/defect/auto-linux-fc288d6a70/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
