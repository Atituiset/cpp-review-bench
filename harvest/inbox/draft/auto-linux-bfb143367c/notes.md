# auto-linux-bfb143367c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7cd80b8168c9269e4b11788ce815c0f446ab5b34](https://github.com/torvalds/linux/commit/7cd80b8168c9269e4b11788ce815c0f446ab5b34) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7cd80b8168c9269e4b11788ce815c0f446ab5b34 (https://github.com/torvalds/linux/commit/7cd80b8168c9269e4b11788ce815c0f446ab5b34)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 499；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7cd80b8168c9269e4b11788ce815c0f446ab5b34 Merge tag 'uml-for-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/uml/linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -490,13 +490,24 @@ static int virtio_pcidev_init_vqs(struct virtio_pcidev_device *dev)
 	return 0;
 }
 
+static void virtio_pcidev_del_vqs(struct virtio_device *vdev,
+				  struct virtio_pcidev_device *dev)
+{
+	void *msg;
+
+	while ((msg = virtqueue_detach_unused_buf(dev->irq_vq)))
+		kfree(msg);
+
+	vdev->config->del_vqs(vdev);
+}
+
 static void __virtio_pcidev_virtio_platform_remove(struct virtio_device *vdev,
 						   struct virtio_pcidev_device *dev)
 {
 	um_pci_platform_device_unregister(&dev->pdev);
 
 	virtio_reset_device(vdev);
-	vdev->config->del_vqs(vdev);
+	virtio_pcidev_del_vqs(vdev, dev);
 
 	kfree(dev);
 }
@@ -526,7 +537,7 @@ static int virtio_pcidev_virtio_platform_probe(struct virtio_device *vdev,
 	um_pci_platform_device_unregister(&dev->pdev);
 err_reset:
 	virtio_reset_device(vdev);
-	vdev->config->del_vqs(vdev);
+	virtio_pcidev_del_vqs(vdev, dev);
 err_free:
 	kfree(dev);
 	return err;
@@ -569,7 +580,7 @@ static int virtio_pcidev_virtio_probe(struct virtio_device *vdev)
 
 err_reset:
 	virtio_reset_device(vdev);
-	vdev->config->del_vqs(vdev);
+	virtio_pcidev_del_vqs(vdev, dev);
 err_free:
 	kfree(dev);
 	return err;
@@ -591,9 +602,9 @@ static void virtio_pcidev_virtio_remove(struct virtio_device *vdev)
 
 	/* Stop all virtqueues */
 	virtio_reset_device(vdev);
+	virtio_pcidev_del_vqs(vdev, dev);
 	dev->cmd_vq = NULL;
 	dev->irq_vq = NULL;
-	vdev->config->del_vqs(vdev);
 
 	kfree(dev);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`del_vqs`
- 外部函数：`kfree`
- 外部函数：`um_pci_platform_device_unregister`
- 外部函数：`virtio_reset_device`
- 大写宏：`NULL`
- 外部类型：`Stop`
- 外部类型：`virtio_device`
- 外部类型：`virtio_pcidev_device`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-bfb143367c` → 本草稿移入 `cases/defect/auto-linux-bfb143367c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
