# auto-linux-5a31c501e0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#bb934c29fda8dcd409145a0deddc1a8095fec930](https://github.com/torvalds/linux/commit/bb934c29fda8dcd409145a0deddc1a8095fec930) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bb934c29fda8dcd409145a0deddc1a8095fec930 (https://github.com/torvalds/linux/commit/bb934c29fda8dcd409145a0deddc1a8095fec930)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 162；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bb934c29fda8dcd409145a0deddc1a8095fec930 Merge tag 'drm-fixes-2026-10-11' of https://gitlab.freedesktop.org/drm/kernel :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -121,6 +121,20 @@ static int  amdgpu_debugfs_process_reg_op(bool read, struct file *f,
 
 	*pos &= (1UL << 22) - 1;
 
+	if (size > PAGE_SIZE)
+		return -EINVAL;
+
+	/*
+	 * Access the user buffer only while none of the locks below are held:
+	 * a fault takes mmap_lock, and grbm_idx_mutex and srbm_mutex nest
+	 * inside it.
+	 */
+	u32 *data __free(kfree) = kmalloc(size, GFP_KERNEL);
+	if (!data)
+		return -ENOMEM;
+	if (!read && copy_from_user(data, buf, size))
+		return -EFAULT;
+
 	r = pm_runtime_get_sync(adev_to_drm(adev)->dev);
 	if (r < 0) {
 		pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
@@ -152,28 +166,16 @@ static int  amdgpu_debugfs_process_reg_op(bool read, struct file *f,
 		mutex_lock(&adev->pm.mutex);
 
 	while (size) {
-		uint32_t value;
-
-		if (read) {
-			value = RREG32(*pos >> 2);
-			r = put_user(value, (uint32_t *)buf);
-		} else {
-			r = get_user(value, (uint32_t *)buf);
-			if (!r)
-				amdgpu_mm_wreg_mmio_rlc(adev, *pos >> 2, value, 0);
-		}
-		if (r) {
-			result = r;
-			goto end;
-		}
+		if (read)
+			data[result >> 2] = RREG32(*pos >> 2);
+		else
+			amdgpu_mm_wreg_mmio_rlc(adev, *pos >> 2, data[result >> 2], 0);
 
 		result += 4;
-		buf += 4;
 		*pos += 4;
 		size -= 4;
 	}
 
-end:
 	if (use_bank) {
 		amdgpu_gfx_select_se_sh(adev, 0xffffffff, 0xffffffff, 0xffffffff, 0);
 		mutex_unlock(&adev->grbm_idx_mutex);
@@ -188,6 +190,10 @@ static int  amdgpu_debugfs_process_reg_op(bool read, struct file *f,
 	pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
 
 	amdgpu_virt_disable_access_debugfs(adev);
+
+	if (read && copy_to_user(buf, data, result))
+		return -EFAULT;
+
 	return result;
 }
 
@@ -238,11 +244,24 @@ static ssize_t amdgpu_debugfs_regs2_op(struct file *f, char __user *buf, u32 off
 	struct amdgpu_device *adev = rd->adev;
 	ssize_t result = 0;
 	int r;
-	uint32_t value;
 
 	if (size & 0x3 || offset & 0x3)
 		return -EINVAL;
 
+	if (size > PAGE_SIZE)
+		return -EINVAL;
+
+	/*
+	 * Access the user buffer only while none of the locks below are held:
+	 * a fault takes mmap_lock, and grbm_idx_mutex and srbm_mutex nest
+	 * inside it.
+	 */
+	u32 *data __free(kfree) = kmalloc(size, GFP_KERNEL);
+	if (!data)
+		return -ENOMEM;
+	if (write_en && copy_from_user(data, buf, size))
+		return -EFAULT;
+
 	r = pm_runtime_get_sync(adev_to_drm(adev)->dev);
 	if (r < 0) {
 		pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
@@ -281,24 +300,16 @@ static ssize_t amdgpu_debugfs_regs2_op(struct file *f, char __user *buf, u32 off
 		mutex_lock(&adev->pm.mutex);
 
 	while (size) {
-		if (!write_en) {
-			value = RREG32(offset >> 2);
-			r = put_user(value, (uint32_t *)buf);
-		} else {
-			r = get_user(value, (uint32_t *)buf);
-			if (!r)
-				amdgpu_mm_wreg_mmio_rlc(adev, offset >> 2, value, rd->id.xcc_id);
-		}
-		if (r) {
-			result = r;
-			goto end;
-		}
+		if (!write_en)
+			data[result >> 2] = RREG32(offset >> 2);
+		else
+			amdgpu_mm_wreg_mmio_rlc(adev, offset >> 2, data[result >> 2],
+						rd->id.xcc_id);
 		offset += 4;
 		size -= 4;
 		result += 4;
-		buf += 4;
 	}
-end:
+
 	if (rd->id.use_grbm) {
 		amdgpu_gfx_select_se_sh(adev, 0xffffffff, 0xffffffff, 0xffffffff, rd->id.xcc_id);
 		mutex_unlock(&adev->grbm_idx_mutex);
@@ -317,6 +328,10 @@ static ssize_t amdgpu_debugfs_regs2_op(struct file *f, char __user *buf, u32 off
 	pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
 
 	amdgpu_virt_disable_access_debugfs(adev);
+
+	if (!write_en && copy_to_user(buf, data, result))
+		return -EFAULT;
+
 	return result;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`adev_to_drm`
- 外部函数：`amdgpu_gfx_select_se_sh`
- 外部函数：`amdgpu_mm_wreg_mmio_rlc`
- 外部函数：`amdgpu_virt_disable_access_debugfs`
- 外部函数：`get_user`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`pm_runtime_get_sync`
- 外部函数：`pm_runtime_put_autosuspend`
- 外部函数：`put_user`
- 大写宏：`EINVAL`
- 大写宏：`RREG32`
- 外部类型：`amdgpu_device`
- 外部类型：`ssize_t`
- 外部类型：`uint32_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-5a31c501e0` → 本草稿移入 `cases/defect/auto-linux-5a31c501e0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
