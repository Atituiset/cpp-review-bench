# auto-linux-bbfbbaa29c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f](https://github.com/torvalds/linux/commit/5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 90 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f (https://github.com/torvalds/linux/commit/5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 102；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f Merge tag 'drm-next-2026-10-21' of https://gitlab.freedesktop.org/drm/kernel :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7,6 +7,7 @@
 #include <drm/drm_device.h>
 #include <drm/drm_gem.h>
 #include <drm/drm_gem_shmem_helper.h>
+#include <drm/drm_managed.h>
 #include <drm/drm_print.h>
 #include <drm/drm_syncobj.h>
 #include <linux/hmm.h>
@@ -99,9 +100,9 @@ static void aie2_job_put(struct amdxdna_sched_job *job)
 static void aie2_hwctx_stop(struct amdxdna_dev *xdna, struct amdxdna_hwctx *hwctx,
 			    struct drm_sched_job *bad_job)
 {
-	drm_sched_stop(&hwctx->priv->sched, bad_job);
+	drm_sched_stop(hwctx->priv->sched, bad_job);
 	aie2_destroy_context(xdna->dev_handle, hwctx);
-	drm_sched_start(&hwctx->priv->sched, 0);
+	drm_sched_start(hwctx->priv->sched, 0);
 }
 
 static int aie2_hwctx_restart(struct amdxdna_dev *xdna, struct amdxdna_hwctx *hwctx)
@@ -289,6 +290,7 @@ aie2_sched_resp_handler(void *handle, void __iomem *data, size_t size)
 	int ret = 0;
 	u32 status;
 
+	amdxdna_io_stats_job_done(job->hwctx->client);
 	cmd_abo = job->cmd_bo;
 
 	if (unlikely(job->job_timeout)) {
@@ -321,6 +323,8 @@ aie2_sched_drvcmd_resp_handler(void *handle, void __iomem *data, size_t size)
 	struct amdxdna_sched_job *job = handle;
 	int ret = 0;
 
+	amdxdna_io_stats_job_done(job->hwctx->client);
+
 	if (unlikely(!data || size != sizeof(u32))) {
 		job->drv_cmd->result = U32_MAX;
 		ret = -EINVAL;
@@ -343,6 +347,7 @@ aie2_sched_cmdlist_resp_handler(void *handle, void __iomem *data, size_t size)
 	u32 cmd_status;
 	int ret = 0;
 
+	amdxdna_io_stats_job_done(job->hwctx->client);
 	cmd_abo = job->cmd_bo;
 
 	if (unlikely(job->job_timeout)) {
@@ -404,6 +409,7 @@ aie2_sched_job_run(struct drm_sched_job *sched_job)
 	kref_get(&job->refcnt);
 	fence = dma_fence_get(job->fence);
 
+	amdxdna_io_stats_job_start(job->hwctx->client);
 	if (job->drv_cmd) {
 		switch (job->drv_cmd->opcode) {
 		case SYNC_DEBUG_BO:
@@ -431,6 +437,7 @@ aie2_sched_job_run(struct drm_sched_job *sched_job)
 
 out:
 	if (ret) {
+		amdxdna_io_stats_job_done(job->hwctx->client);
 		dma_fence_put(job->fence);
 		aie2_job_put(job);
 		mmput(job->mm);
@@ -659,10 +666,19 @@ static void aie2_ctx_syncobj_destroy(struct amdxdna_hwctx *hwctx)
 	drm_syncobj_put(hwctx->priv->syncobj);
 }
 
-int aie2_hwctx_init(struct amdxdna_hwctx *hwctx)
+void aie2_hwctx_sched_fini(struct amdxdna_dev_hdl *ndev)
 {
-	struct amdxdna_client *client = hwctx->client;
-	struct amdxdna_dev *xdna = client->xdna;
+	int i;
+
+	for (i = 0; i < ndev->priv->hwctx_limit; i++)
+		drm_sched_fini(&ndev->hwctx_sched[i]);
+
+	ida_destroy(&ndev->hwctx_sched_ida);
+}
+
+int aie2_hwctx_sched_init(struct amdxdna_dev_hdl *ndev)
+{
+	struct amdxdna_dev *xdna = ndev->aie.xdna;
 	const struct drm_sched_init_args args = {
 		.ops = &sched_ops,
 		.num_rqs = DRM_SCHED_PRIORITY_COUNT,
@@ -673,7 +689,55 @@ int aie2_hwctx_init(struct amdxdna_hwctx *hwctx)
 		.name = "amdxdna_js",
 		.dev = xdna->ddev.dev,
 	};
-	struct drm_gpu_scheduler *sched;
+	int i, ret;
+
+	ndev->hwctx_sched = drmm_kcalloc(&xdna->ddev, ndev->priv->hwctx_limit,
+					 sizeof(*ndev->hwctx_sched), GFP_KERNEL);
+	if (!ndev->hwctx_sched)
+		return -ENOMEM;
+
+	for (i = 0; i < ndev->priv->hwctx_limit; i++) {
+		ret = drm_sched_init(&ndev->hwctx_sched[i], &args);
+		if (ret) {
+			XDNA_ERR(xdna, "Failed to init DRM scheduler. ret %d", ret);
+			goto fini_sched;
+		}
+	}
+
+	ida_init(&ndev->hwctx_sched_ida);
+
+	return 0;
+
+fini_sched:
+	while (--i >= 0)
+		drm_sched_fini(&ndev->hwctx_sched[i]);
+
+	return ret;
+}
+
+static struct drm_gpu_scheduler *aie2_hwctx_sched_alloc(struct amdxdna_dev_hdl *ndev)
+{
+	int ret;
+
+	ret = ida_alloc_range(&ndev->hwctx_sched_ida, 0,
+			      ndev->priv->hwctx_limit - 1, GFP_KERNEL);
+	if (ret < 0)
+		return ERR_PTR(ret);
+
+	return &ndev->hwctx_sched[ret];
+}
+
+static void aie2_hwctx_sched_free(struct amdxdna_dev_hdl *ndev,
+				  struct drm_gpu_scheduler *sched)
+{
+	ida_free(&ndev->hwctx_sched_ida, sched - ndev->hwctx_sched);
+}
+
+int aie2_hwctx_init(struct amdxdna_hwctx *hwctx)
+{
+	struct amdxdna_client *client = hwctx->client;
+	struct amdxdna_dev
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`aie2_add_host_buf`
- 外部函数：`aie2_config_cu`
- 外部函数：`aie2_create_context`
- 外部函数：`aie2_destroy_context`
- 外部函数：`aie2_map_host_buf`
- 外部函数：`amdxdna_obj_dma_addr`
- 外部函数：`amdxdna_sched_job_cleanup`
- 外部函数：`atomic64_inc`
- 外部函数：`container_of`
- 外部函数：`dma_fence_chain_contained`
- 外部函数：`dma_fence_chain_find_seqno`
- 外部函数：`dma_fence_get`
- 外部函数：`dma_fence_put`
- 外部函数：`dma_fence_wait_timeout`
- 外部函数：`drm_sched_start`
- 外部函数：`drm_sched_stop`
- 外部函数：`drm_syncobj_fence_get`
- 外部函数：`kfree`
- 外部函数：`kmalloc_array`
- 外部函数：`kref_get`
- 外部函数：`kref_put`
- 外部函数：`mmput`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`print_hex_dump_debug`
- 外部函数：`unlikely`
- 外部函数：`wake_up`
- 外部函数：`xrs_release_resource`
- 大写宏：`AIE`
- 大写宏：`COL_ALIGN_NATURE`
- 大写宏：`DUMP_PREFIX_OFFSET`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`NULL`
- 大写宏：`SYNC_DEBUG_BO`
- 大写宏：`U32_MAX`
- 大写宏：`XDNA_DBG`
- 大写宏：`XDNA_ERR`
- 大写宏：`XDNA_WARN`
- 外部类型：`Add`
- 外部类型：`Config`
- 外部类型：`Core`
- 外部类型：`Create`
- 外部类型：`Force`
- 外部类型：`In`
- 外部类型：`Invalid`
- 外部类型：`Map`
- 外部类型：`Number`
- 外部类型：`Release`
- 外部类型：`Start`
- 外部类型：`Wait`
- 外部类型：`amdxdna_dev`
- 外部类型：`amdxdna_dev_hdl`
- 外部类型：`amdxdna_drv_cmd`
- 外部类型：`amdxdna_gem_obj`
- 外部类型：`amdxdna_hwctx`
- 外部类型：`amdxdna_sched_job`
- 外部类型：`dma_fence`
- 外部类型：`drm_sched_job`
- 外部类型：`kref`
- 外部类型：`uintptr_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-bbfbbaa29c` → 本草稿移入 `cases/defect/auto-linux-bbfbbaa29c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
