# auto-linux-40b5f2ba19

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9a3415a159e7615a7043280ccc4ac310199df697](https://github.com/torvalds/linux/commit/9a3415a159e7615a7043280ccc4ac310199df697) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 81 |
| 编译错误数（gcc syntax-only） | 33（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9a3415a159e7615a7043280ccc4ac310199df697 (https://github.com/torvalds/linux/commit/9a3415a159e7615a7043280ccc4ac310199df697)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 672；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9a3415a159e7615a7043280ccc4ac310199df697 Merge tag 'drm-misc-fixes-2026-11-27' of https://gitlab.freedesktop.org/drm/misc/kernel into drm-fixes :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -669,10 +669,8 @@ static __always_inline int v3d_pm_runtime_put(struct v3d_dev *v3d)
 }
 
 /* v3d_sched.c */
-void v3d_timestamp_query_info_free(struct v3d_timestamp_query_info *query_info,
-				   unsigned int count);
-void v3d_performance_query_info_free(struct v3d_performance_query_info *query_info,
-				     unsigned int count);
+void v3d_timestamp_query_info_free(struct v3d_timestamp_query_info *query_info);
+void v3d_performance_query_info_free(struct v3d_performance_query_info *query_info);
 struct v3d_stats *v3d_stats_alloc(void);
 void v3d_stats_release(struct kref *refcount);
 void v3d_job_update_stats(struct v3d_job *job);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__counted_by`
- 外部函数：`concurrently`
- 外部函数：`monitor`
- 外部函数：`representation`
- 外部函数：`v3d_job_update_stats`
- 外部函数：`v3d_performance_query_info_free`
- 外部函数：`v3d_stats_alloc`
- 外部函数：`v3d_stats_release`
- 外部函数：`v3d_timestamp_query_info_free`
- 外部函数：`void`
- 外部函数：`window`
- 大写宏：`CPU`
- 大写宏：`DMA`
- 大写宏：`DRM_V3D_MAX_PERF_COUNTERS`
- 大写宏：`GPU`
- 大写宏：`IRQ`
- 大写宏：`MMU`
- 大写宏：`NULL`
- 大写宏：`V3D`
- 大写宏：`V3D_CPU`
- 大写宏：`V3D_GEN_33`
- 大写宏：`V3D_GEN_41`
- 大写宏：`V3D_GEN_42`
- 大写宏：`V3D_GEN_71`
- 大写宏：`V3D_MAX_IRQS`
- 外部类型：`All`
- 外部类型：`Allocator`
- 外部类型：`BOs`
- 外部类型：`Callback`
- 外部类型：`Counters`
- 外部类型：`Currently`
- 外部类型：`Events`
- 外部类型：`File`
- 外部类型：`Finished`
- 外部类型：`Global`
- 外部类型：`However`
- 外部类型：`IDs`
- 外部类型：`IRQs`
- 外部类型：`It`
- 外部类型：`Jobs`
- 外部类型：`Lock`
- 外部类型：`Note`
- 外部类型：`Number`
- 外部类型：`Offset`
- 外部类型：`Ordered`
- 外部类型：`Perfmon`
- 外部类型：`Performance`
- 外部类型：`Pointer`
- 外部类型：`Pointers`
- 外部类型：`Protects`
- 外部类型：`Short`
- 外部类型：`Storage`
- 外部类型：`Stores`
- 外部类型：`Syncobj`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Tracks`
- 外部类型：`Virtual`
- 外部类型：`When`
- 外部类型：`Whether`
- 外部类型：`atomic_t`
- 外部类型：`clk`
- 外部类型：`dma_addr_t`
- 外部类型：`dma_fence`
- 外部类型：`drm_device`
- 外部类型：`drm_gem_object`
- 外部类型：`drm_gpu_scheduler`
- 外部类型：`drm_mm`
- 外部类型：`drm_sched_entity`
- 外部类型：`drm_sched_job`
- 外部类型：`drm_syncobj`
- 外部类型：`kref`
- 外部类型：`mutex`
- 外部类型：`refcount_t`
- 外部类型：`reset_control`
- 外部类型：`seqcount_t`
- 外部类型：`spinlock_t`
- 外部类型：`v3d_perfmon_info`
- 外部类型：`v3d_queue`
- 外部类型：`work_struct`
- 外部类型：`workqueue_struct`
- 外部类型：`xarray`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-40b5f2ba19` → 本草稿移入 `cases/defect/auto-linux-40b5f2ba19/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
