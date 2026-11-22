# auto-linux-1dc0a8d585

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
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 26（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c44104f3642da720164042623e4832836aefdd8e (https://github.com/torvalds/linux/commit/c44104f3642da720164042623e4832836aefdd8e)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 121；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c44104f3642da720164042623e4832836aefdd8e Merge tag 'drm-fixes-2026-11-21' of https://gitlab.freedesktop.org/drm/kernel :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -54,8 +54,11 @@ static int vmw_gmrid_man_get_node(struct ttm_resource_manager *man,
 				  const struct ttm_place *place,
 				  struct ttm_resource **res)
 {
+	struct vmw_private *vmw = vmw_priv_from_ttm(bo->bdev);
 	struct vmwgfx_gmrid_man *gman = to_gmrid_manager(man);
-	int id;
+	u32 max_gmr_pages = 0;
+	bool mob_overflow = false;
+	int id, ret = 0;
 
 	*res = kmalloc_obj(**res);
 	if (!*res)
@@ -85,43 +88,53 @@ static int vmw_gmrid_man_get_node(struct ttm_resource_manager *man,
 			const unsigned long max_graphics_pages = totalram_pages() / 2;
 			uint32_t new_max_pages = 0;
 
-			DRM_WARN("vmwgfx: mob memory overflow. Consider increasing guest RAM and graphicsMemory.\n");
-			vmw_host_printf("vmwgfx, warning: mob memory overflow. Consider increasing guest RAM and graphicsMemory.\n");
+			mob_overflow = true;
+			drm_warn(&vmw->drm,
+				 "vmwgfx: mob memory overflow. Consider increasing guest RAM and graphicsMemory.\n");
 
 			if (gman->max_gmr_pages > (max_graphics_pages / 2)) {
-				DRM_WARN("vmwgfx: guest requires more than half of RAM for graphics.\n");
+				drm_warn(&vmw->drm,
+					 "vmwgfx: guest requires more than half of RAM for graphics.\n");
 				new_max_pages = max_graphics_pages;
 			} else
 				new_max_pages = gman->max_gmr_pages * 2;
 			if (new_max_pages > gman->max_gmr_pages && new_max_pages >= gman->used_gmr_pages) {
-				DRM_WARN("vmwgfx: increasing guest mob limits to %u KiB.\n",
-					 ((new_max_pages) << (PAGE_SHIFT - 10)));
+				drm_warn(&vmw->drm,
+					 "vmwgfx: increasing guest mob limits to %u KiB.\n",
+					 new_max_pages << (PAGE_SHIFT - 10));
 
 				gman->max_gmr_pages = new_max_pages;
 			} else {
-				char buf[256];
-				snprintf(buf, sizeof(buf),
-					 "vmwgfx, error: guest graphics is out of memory (mob limit at: %u KiB).\n",
-					 ((gman->max_gmr_pages) << (PAGE_SHIFT - 10)));
-				vmw_host_printf(buf);
-				DRM_WARN("%s", buf);
-				goto nospace;
+				max_gmr_pages = gman->max_gmr_pages;
+				gman->used_gmr_pages -= PFN_UP((*res)->size);
+				ret = -ENOSPC;
+				goto out_unlock;
 			}
 		}
 	}
 
 	(*res)->start = id;
 
+out_unlock:
 	spin_unlock(&gman->lock);
-	return 0;
 
-nospace:
-	gman->used_gmr_pages -= PFN_UP((*res)->size);
-	spin_unlock(&gman->lock);
-	ida_free(&gman->gmr_ida, id);
-	ttm_resource_fini(man, *res);
-	kfree(*res);
-	return -ENOSPC;
+	/* Host logging allocates with GFP_KERNEL and must run unlocked. */
+	if (mob_overflow)
+		vmw_host_printf("%s",
+				"vmwgfx, warning: mob memory overflow. Consider increasing guest RAM and graphicsMemory.\n");
+	if (ret) {
+		char buf[256];
+
+		snprintf(buf, sizeof(buf),
+			 "vmwgfx, error: guest graphics is out of memory (mob limit at: %u KiB).\n",
+			 max_gmr_pages << (PAGE_SHIFT - 10));
+		vmw_host_printf("%s", buf);
+		drm_warn(&vmw->drm, "%s", buf);
+		ida_free(&gman->gmr_ida, id);
+		ttm_resource_fini(man, *res);
+		kfree(*res);
+	}
+	return ret;
 }
 
 static void vmw_gmrid_man_put_node(struct ttm_resource_manager *man,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`container_of`
- 外部函数：`ida_free`
- 外部函数：`kfree`
- 外部函数：`kmalloc_obj`
- 外部函数：`memory`
- 外部函数：`snprintf`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`totalram_pages`
- 外部函数：`ttm_resource_fini`
- 外部函数：`vmw_host_printf`
- 大写宏：`DRM_WARN`
- 大写宏：`ENOSPC`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PFN_UP`
- 大写宏：`RAM`
- 外部类型：`Consider`
- 外部类型：`KiB`
- 外部类型：`ida`
- 外部类型：`spinlock_t`
- 外部类型：`ttm_place`
- 外部类型：`ttm_resource`
- 外部类型：`ttm_resource_manager`
- 外部类型：`uint32_t`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-1dc0a8d585` → 本草稿移入 `cases/defect/auto-linux-1dc0a8d585/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
