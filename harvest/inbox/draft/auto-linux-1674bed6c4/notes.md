# auto-linux-1674bed6c4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ae3690eb2a994dd74a43639d6bf800b51f18b48c](https://github.com/torvalds/linux/commit/ae3690eb2a994dd74a43639d6bf800b51f18b48c) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ae3690eb2a994dd74a43639d6bf800b51f18b48c (https://github.com/torvalds/linux/commit/ae3690eb2a994dd74a43639d6bf800b51f18b48c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2503；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ae3690eb2a994dd74a43639d6bf800b51f18b48c Merge tag 'dmaengine-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/vkoul/dmaengine :: PR 修复动作推断：修复前缓冲区溢出（加长度约束）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2500,8 +2500,8 @@ DEFINE_SHOW_ATTRIBUTE(pl08x_debugfs);
 static void init_pl08x_debugfs(struct pl08x_driver_data *pl08x)
 {
 	/* Expose a simple debugfs interface to view all clocks */
-	debugfs_create_file(dev_name(&pl08x->adev->dev), S_IFREG | S_IRUGO,
-			    NULL, pl08x, &pl08x_debugfs_fops);
+	debugfs_create_file("stats", 0444, pl08x->memcpy.dbg_dev_root,
+			    pl08x, &pl08x_debugfs_fops);
 }
 
 #else
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`debugfs_create_file`
- 外部函数：`dev_name`
- 大写宏：`NULL`
- 大写宏：`PL08X_CHAN_IDLE`
- 大写宏：`PL08X_CHAN_PAUSED`
- 大写宏：`PL08X_CHAN_RUNNING`
- 大写宏：`PL08X_CHAN_WAITING`
- 大写宏：`S_IFREG`
- 大写宏：`S_IRUGO`
- 外部类型：`Default`
- 外部类型：`Expose`
- 外部类型：`LLIs`
- 外部类型：`Other`
- 外部类型：`Settings`
- 外部类型：`amba_device`
- 外部类型：`dma_addr_t`
- 外部类型：`dma_device`
- 外部类型：`dma_pool`
- 外部类型：`dma_slave_config`
- 外部类型：`list_head`
- 外部类型：`pl08x_channel_data`
- 外部类型：`pl08x_platform_data`
- 外部类型：`spinlock_t`
- 外部类型：`virt_dma_chan`
- 外部类型：`virt_dma_desc`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1674bed6c4` → 本草稿移入 `cases/defect/auto-linux-1674bed6c4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
