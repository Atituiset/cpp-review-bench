# auto-linux-1558c2f05f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ce1e0223d8ad4211275c82a17ed6d43ab81e13d9](https://github.com/torvalds/linux/commit/ce1e0223d8ad4211275c82a17ed6d43ab81e13d9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ce1e0223d8ad4211275c82a17ed6d43ab81e13d9 (https://github.com/torvalds/linux/commit/ce1e0223d8ad4211275c82a17ed6d43ab81e13d9)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 101；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ce1e0223d8ad4211275c82a17ed6d43ab81e13d9 Merge tag 'devicetree-fixes-for-7.3-2' of git://git.kernel.org/pub/scm/linux/kernel/git/robh/linux :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -96,9 +96,17 @@ static bool __of_node_is_type(const struct device_node *np, const char *type)
 	return !strcmp(match, type);
 }
 
+static bool of_coreboot_present(void)
+{
+	struct device_node *np __free(device_node) =
+		of_find_compatible_node(NULL, NULL, "coreboot");
+
+	return np;
+}
+
 #define EXCLUDED_DEFAULT_CELLS_PLATFORMS ( \
 	IS_ENABLED(CONFIG_SPARC) || \
-	of_find_compatible_node(NULL, NULL, "coreboot") \
+	of_coreboot_present() \
 )
 
 int of_bus_n_addr_cells(struct device_node *np)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`kbasename`
- 外部函数：`of_bus_n_addr_cells`
- 外部函数：`of_compat_cmp`
- 外部函数：`of_node_get`
- 外部函数：`of_node_put`
- 外部函数：`of_prop_cmp`
- 外部函数：`of_property_read_u32`
- 外部函数：`raw_spin_lock_irqsave`
- 外部函数：`raw_spin_unlock_irqrestore`
- 外部函数：`strchrnul`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 外部函数：`strnlen`
- 大写宏：`CONFIG_SPARC`
- 大写宏：`INT_MAX`
- 大写宏：`IS_ENABLED`
- 大写宏：`NULL`
- 大写宏：`OF_ROOT_NODE_ADDR_CELLS_DEFAULT`
- 大写宏：`WARN_ONCE`
- 外部类型：`Any`
- 外部类型：`Compatible`
- 外部类型：`Default`
- 外部类型：`Matching`
- 外部类型：`Missing`
- 外部类型：`device_node`
- 外部类型：`property`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1558c2f05f` → 本草稿移入 `cases/defect/auto-linux-1558c2f05f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
