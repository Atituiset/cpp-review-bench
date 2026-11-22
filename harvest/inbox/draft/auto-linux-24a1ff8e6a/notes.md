# auto-linux-24a1ff8e6a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c44104f3642da720164042623e4832836aefdd8e](https://github.com/torvalds/linux/commit/c44104f3642da720164042623e4832836aefdd8e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c44104f3642da720164042623e4832836aefdd8e (https://github.com/torvalds/linux/commit/c44104f3642da720164042623e4832836aefdd8e)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 76；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c44104f3642da720164042623e4832836aefdd8e Merge tag 'drm-fixes-2026-11-21' of https://gitlab.freedesktop.org/drm/kernel :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -73,10 +73,13 @@ static struct sg_table *vmw_gem_object_get_sg_table(struct drm_gem_object *obj)
 	struct vmw_ttm_tt *vmw_tt =
 		container_of(bo->ttm, struct vmw_ttm_tt, dma_ttm);
 
-	if (vmw_tt->vsgt.sgt)
-		return vmw_tt->vsgt.sgt;
-
-	return drm_prime_pages_to_sg(obj->dev, vmw_tt->dma_ttm.pages, vmw_tt->dma_ttm.num_pages);
+	/*
+	 * Do not return &vmw_tt->sgt: the core owns what this returns and
+	 * drm_gem_unmap_dma_buf() sg_free_table()s and kfree()s it, but that
+	 * sg_table is embedded in the vmw_ttm_tt allocation.
+	 */
+	return drm_prime_pages_to_sg(obj->dev, vmw_tt->dma_ttm.pages,
+				     vmw_tt->dma_ttm.num_pages);
 }
 
 static int vmw_gem_vmap(struct drm_gem_object *obj, struct iosys_map *map)
@@ -184,7 +187,7 @@ struct drm_gem_object *vmw_prime_import_sg_table(struct drm_device *dev,
 {
 	int ret;
 	struct vmw_private *dev_priv = vmw_priv(dev);
-	struct drm_gem_object *gem = NULL;
+	struct drm_gem_object *gem;
 	struct vmw_bo *vbo;
 	struct vmw_bo_params params = {
 		.domain = (dev_priv->has_mob) ? VMW_BO_DOMAIN_SYS : VMW_BO_DOMAIN_VRAM,
@@ -201,8 +204,10 @@ struct drm_gem_object *vmw_prime_import_sg_table(struct drm_device *dev,
 	dma_resv_lock(params.resv, NULL);
 
 	ret = vmw_bo_create(dev_priv, &params, &vbo);
-	if (ret != 0)
+	if (ret != 0) {
+		gem = ERR_PTR(ret);
 		goto out_no_bo;
+	}
 
 	vbo->tbo.base.funcs = &vmw_gem_object_funcs;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`container_of`
- 外部函数：`dma_resv_lock`
- 外部函数：`drm_prime_pages_to_sg`
- 外部函数：`vmw_bo_create`
- 外部函数：`vmw_gem_vmap`
- 外部函数：`vmw_priv`
- 大写宏：`NULL`
- 大写宏：`VMW_BO_DOMAIN_SYS`
- 大写宏：`VMW_BO_DOMAIN_VRAM`
- 外部类型：`drm_gem_object`
- 外部类型：`iosys_map`
- 外部类型：`vmw_bo`
- 外部类型：`vmw_bo_params`
- 外部类型：`vmw_private`
- 外部类型：`vmw_ttm_tt`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-24a1ff8e6a` → 本草稿移入 `cases/defect/auto-linux-24a1ff8e6a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
