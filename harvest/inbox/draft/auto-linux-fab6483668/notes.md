# auto-linux-fab6483668

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#543b7d2ca09b4b08d518d668ac2fdc361f231785](https://github.com/torvalds/linux/commit/543b7d2ca09b4b08d518d668ac2fdc361f231785) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #543b7d2ca09b4b08d518d668ac2fdc361f231785 (https://github.com/torvalds/linux/commit/543b7d2ca09b4b08d518d668ac2fdc361f231785)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 978；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 543b7d2ca09b4b08d518d668ac2fdc361f231785 Merge tag 'drm-fixes-2026-11-07' of https://gitlab.freedesktop.org/drm/kernel :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -975,11 +975,11 @@ static int kfd_mem_attach(struct amdgpu_device *adev, struct kgd_mem *mem,
 			ret = amdgpu_vm_lock_pd(vm, &exec, 0);
 			drm_exec_retry_on_contention(&exec);
 			if (unlikely(ret))
-				goto unwind;
+				goto unlock;
 			ret = drm_exec_lock_obj(&exec, &bo[i]->tbo.base);
 			drm_exec_retry_on_contention(&exec);
 			if (unlikely(ret))
-				goto unwind;
+				goto unlock;
 		}
 
 		bo_va = amdgpu_vm_bo_find(vm, bo[i]);
@@ -1005,6 +1005,8 @@ static int kfd_mem_attach(struct amdgpu_device *adev, struct kgd_mem *mem,
 
 	return 0;
 
+unlock:
+	drm_exec_fini(&exec);
 unwind:
 	for (; i >= 0; i--) {
 		if (!attachment[i])
@@ -2261,11 +2263,13 @@ int amdgpu_amdkfd_gpuvm_sync_memory(
  * amdgpu_amdkfd_map_gtt_bo_to_gart - Map BO to GART and increment reference count
  * @bo: Buffer object to be mapped
  * @bo_gart: Return bo reference
+ * @process_info: KFD process info of the queue the BO is for
  *
  * Before return, bo reference count is incremented. To release the reference and unpin/
  * unmap the BO, call amdgpu_amdkfd_free_kernel_mem.
  */
-int amdgpu_amdkfd_map_gtt_bo_to_gart(struct amdgpu_bo *bo, struct amdgpu_bo **bo_gart)
+int amdgpu_amdkfd_map_gtt_bo_to_gart(struct amdgpu_bo *bo, struct amdgpu_bo **bo_gart,
+				     struct amdkfd_process_info *process_info)
 {
 	int ret;
 
@@ -2287,8 +2291,8 @@ int amdgpu_amdkfd_map_gtt_bo_to_gart(struct amdgpu_bo *bo, struct amdgpu_bo **bo
 		goto err_map_bo_gart_failed;
 	}
 
-	amdgpu_amdkfd_remove_eviction_fence(
-		bo, bo->vm_bo->vm->process_info->eviction_fence);
+	/* bo->vm_bo can be the VM of any file which opened the BO */
+	amdgpu_amdkfd_remove_eviction_fence(bo, process_info->eviction_fence);
 
 	amdgpu_bo_unreserve(bo);
 
@@ -2473,6 +2477,9 @@ static int import_obj_create(struct amdgpu_device *adev,
 
 err_remove_mem:
 	remove_kgd_mem_from_kfd_bo_list(*mem, avm->process_info);
+	amdgpu_sync_free(&(*mem)->sync);
+	mutex_destroy(&(*mem)->lock);
+	dma_buf_put(dma_buf);
 	drm_vma_node_revoke(&obj->vma_node, drm_priv);
 err_free_mem:
 	kfree(*mem);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`amdgpu_bo_pin`
- 外部函数：`amdgpu_bo_ref`
- 外部函数：`amdgpu_bo_reserve`
- 外部函数：`amdgpu_bo_unpin`
- 外部函数：`amdgpu_bo_unreserve`
- 外部函数：`amdgpu_ttm_alloc_gart`
- 外部函数：`amdgpu_vm_bo_find`
- 外部函数：`amdgpu_vm_lock_pd`
- 外部函数：`dma_fence_get_stub`
- 外部函数：`dma_fence_put`
- 外部函数：`dma_resv_replace_fences`
- 外部函数：`drm_exec_lock_obj`
- 外部函数：`drm_exec_retry_on_contention`
- 外部函数：`drm_vma_node_revoke`
- 外部函数：`kfree`
- 外部函数：`list_del`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`pr_err`
- 外部函数：`remove_kgd_mem_from_kfd_bo_list`
- 外部函数：`unlikely`
- 大写宏：`AMDGPU_GEM_DOMAIN_GTT`
- 大写宏：`DMA_RESV_USAGE_BOOKKEEP`
- 大写宏：`EINVAL`
- 大写宏：`GART`
- 大写宏：`TLB`
- 大写宏：`TODO`
- 外部类型：`Before`
- 外部类型：`Buffer`
- 外部类型：`Failed`
- 外部类型：`Instead`
- 外部类型：`Map`
- 外部类型：`Return`
- 外部类型：`To`
- 外部类型：`amdgpu_amdkfd_fence`
- 外部类型：`amdgpu_bo`
- 外部类型：`amdkfd_process_info`
- 外部类型：`dma_fence`
- 外部类型：`kgd_mem`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-fab6483668` → 本草稿移入 `cases/defect/auto-linux-fab6483668/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
