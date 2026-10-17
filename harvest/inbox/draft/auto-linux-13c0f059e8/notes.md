# auto-linux-13c0f059e8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#f91a4ac69629e1479745e82100863157e6967ec0](https://github.com/torvalds/linux/commit/f91a4ac69629e1479745e82100863157e6967ec0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 121 |
| 编译错误数（gcc syntax-only） | 160（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f91a4ac69629e1479745e82100863157e6967ec0 (https://github.com/torvalds/linux/commit/f91a4ac69629e1479745e82100863157e6967ec0)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 1468；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f91a4ac69629e1479745e82100863157e6967ec0 Merge tag 'amd-drm-fixes-7.3-2026-10-15' of https://gitlab.freedesktop.org/drm/amdgpu/kernel into drm-fixes :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1431,6 +1431,7 @@ static void amdgpu_cs_parser_fini(struct amdgpu_cs_parser *parser)
 int amdgpu_cs_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
 {
 	struct amdgpu_device *adev = drm_to_adev(dev);
+	struct amdgpu_bo_list_entry *e;
 	struct amdgpu_cs_parser parser;
 	int r;
 
@@ -1465,25 +1466,31 @@ int amdgpu_cs_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
 
 	r = amdgpu_cs_patch_jobs(&parser);
 	if (r)
-		goto error_fini;
+		goto error_free_user_pages;
 
 	r = amdgpu_cs_vm_handling(&parser);
 	if (r)
-		goto error_fini;
+		goto error_free_user_pages;
 
 	r = amdgpu_cs_sync_rings(&parser);
 	if (r)
-		goto error_fini;
+		goto error_free_user_pages;
 
 	trace_amdgpu_cs_ibs(&parser);
 
 	r = amdgpu_cs_submit(&parser, data);
 	if (r)
-		goto error_fini;
+		goto error_free_user_pages;
 
 	amdgpu_cs_parser_fini(&parser);
 	return 0;
 
+error_free_user_pages:
+	/* amdgpu_cs_submit() only drops the ranges once it got that far. */
+	amdgpu_bo_list_for_each_userptr_entry(e, parser.bo_list) {
+		amdgpu_hmm_range_free(e->range);
+		e->range = NULL;
+	}
 error_fini:
 	amdgpu_cs_parser_fini(&parser);
 	return r;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`adev_to_drm`
- 外部函数：`amdgpu_bo_explicit_sync`
- 外部函数：`amdgpu_bo_kmap`
- 外部函数：`amdgpu_bo_kunmap`
- 外部函数：`amdgpu_bo_list_put`
- 外部函数：`amdgpu_bo_placement_from_domain`
- 外部函数：`amdgpu_bo_unref`
- 外部函数：`amdgpu_cs_parser_bos`
- 外部函数：`amdgpu_ctx_add_fence`
- 外部函数：`amdgpu_ctx_put`
- 外部函数：`amdgpu_ctx_wait_prev_fence`
- 外部函数：`amdgpu_gmc_pd_addr`
- 外部函数：`amdgpu_hmm_range_free`
- 外部函数：`amdgpu_hmm_range_valid`
- 外部函数：`amdgpu_job_free`
- 外部函数：`amdgpu_job_free_resources`
- 外部函数：`amdgpu_job_ring`
- 外部函数：`amdgpu_job_set_gang_leader`
- 外部函数：`amdgpu_ring_parse_cs`
- 外部函数：`amdgpu_ring_patch_cs_in_place`
- 外部函数：`amdgpu_sa_bo_gpu_addr`
- 外部函数：`amdgpu_sync_fence`
- 外部函数：`amdgpu_sync_free`
- 外部函数：`amdgpu_sync_get_fence`
- 外部函数：`amdgpu_sync_push_to_job`
- 外部函数：`amdgpu_sync_resv`
- 外部函数：`amdgpu_ttm_alloc_gart`
- 外部函数：`amdgpu_vm_bo_invalidate`
- 外部函数：`amdgpu_vm_bo_lookup_mapping`
- 外部函数：`amdgpu_vm_bo_trace_cs`
- 外部函数：`amdgpu_vm_bo_update`
- 外部函数：`amdgpu_vm_clear_freed`
- 外部函数：`amdgpu_vm_handle_moved`
- 外部函数：`amdgpu_vm_move_to_lru_tail`
- 外部函数：`amdgpu_vm_ready`
- 外部函数：`amdgpu_vm_update_pdes`
- 外部函数：`amdgpu_vmid_uses_reserved`
- 外部函数：`dma_fence_get`
- 外部函数：`dma_fence_put`
- 外部函数：`dma_resv_add_fence`
- 外部函数：`dma_resv_locking_ctx`
- 外部函数：`drm_err`
- 外部函数：`drm_exec_fini`
- 外部函数：`drm_exec_ticket`
- 外部函数：`drm_sched_entity_push_job`
- 外部函数：`drm_sched_job_add_dependency`
- 外部函数：`drm_sched_job_arm`
- 外部函数：`drm_syncobj_add_point`
- 外部函数：`drm_syncobj_put`
- 外部函数：`drm_syncobj_replace_fence`
- 外部函数：`drm_to_adev`
- 外部函数：`gem_to_amdgpu_bo`
- 外部函数：`kfree`
- 外部函数：`kvfree`
- 外部函数：`list_empty`
- 外部函数：`memcpy`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`to_amdgpu_ring`
- 外部函数：`to_drm_sched_fence`
- 外部函数：`trace_amdgpu_cs`
- 外部函数：`trace_amdgpu_cs_enabled`
- 外部函数：`trace_amdgpu_cs_ioctl`
- 外部函数：`ttm_bo_move_to_lru_tail_unlocked`
- 外部函数：`ttm_bo_validate`
- 大写宏：`AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS`
- 大写宏：`AMDGPU_GMC_HOLE_MASK`
- 大写宏：`AMDGPU_GPU_PAGE_SIZE`
- 大写宏：`AMDGPU_PREAMBLE_IB_PRESENT`
- 大写宏：`AMDGPU_PREAMBLE_IB_PRESENT_FIRST`
- 大写宏：`AMDGPU_SYNC_EXPLICIT`
- 大写宏：`AMDGPU_SYNC_NE_OWNER`
- 大写宏：`DMA_RESV_USAGE_READ`
- 大写宏：`DMA_RESV_USAGE_WRITE`
- 大写宏：`EAGAIN`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`ERESTARTSYS`
- 大写宏：`FIXME`
- 大写宏：`GFP_KERNEL`
- 大写宏：`NULL`
- 大写宏：`READ`
- 大写宏：`TTM_PL_FLAG_CONTIGUOUS`
- 大写宏：`TTM_PL_VRAM`
- 大写宏：`UVD`
- 大写宏：`VCE`
- 大写宏：`VRAM`
- 外部类型：`BOs`
- 外部类型：`But`
- 外部类型：`Double`
- 外部类型：`Everybody`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Invalidate`
- 外部类型：`Make`
- 外部类型：`No`
- 外部类型：`Only`
- 外部类型：`The`
- 外部类型：`VMIDs`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`amdgpu_bo`
- 外部类型：`amdgpu_bo_list_entry`
- 外部类型：`amdgpu_bo_va`
- 外部类型：`amdgpu_bo_va_mapping`
- 外部类型：`amdgpu_cs_parser`
- 外部类型：`amdgpu_device`
- 外部类型：`amdgpu_fpriv`
- 外部类型：`amdgpu_ib`
- 外部类型：`amdgpu_job`
- 外部类型：`amdgpu_ring`
- 外部类型：`amdgpu_sync_mode`
- 外部类型：`amdgpu_vm`
- 外部类型：`dma_fence`
- 外部类型：`dma_resv`
- 外部类型：`drm_amdgpu_cs`
- 外部类型：`drm_device`
- 外部类型：`drm_file`
- 外部类型：`drm_gem_object`
- 外部类型：`drm_gpu_scheduler`
- 外部类型：`drm_sched_entity`
- 外部类型：`drm_sched_fence`
- 外部类型：`ttm_operation_ctx`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-13c0f059e8` → 本草稿移入 `cases/defect/auto-linux-13c0f059e8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
