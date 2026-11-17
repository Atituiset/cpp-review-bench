# auto-linux-53fcf34ce5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#666539eecdf4db623f5e3c3e55cf9f2dfad24aa4](https://github.com/torvalds/linux/commit/666539eecdf4db623f5e3c3e55cf9f2dfad24aa4) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 95 |
| 编译错误数（gcc syntax-only） | 178（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #666539eecdf4db623f5e3c3e55cf9f2dfad24aa4 (https://github.com/torvalds/linux/commit/666539eecdf4db623f5e3c3e55cf9f2dfad24aa4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 541；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 666539eecdf4db623f5e3c3e55cf9f2dfad24aa4 Merge tag 'assoc-array-fixes-v2' :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -470,33 +470,32 @@ static bool assoc_array_insert_in_empty_tree(struct assoc_array_edit *edit)
 }
 
 /*
- * Handle insertion into a terminal node.
+ * The pointer that leads to a node or shortcut with the given back pointer
+ * and parent slot: the root, a slot in a node, or a shortcut's next node.
  */
-static bool assoc_array_insert_into_terminal_node(struct assoc_array_edit *edit,
-						  const struct assoc_array_ops *ops,
-						  const void *index_key,
-						  struct assoc_array_walk_result *result)
+static struct assoc_array_ptr **assoc_array_parent_link(struct assoc_array *array,
+							struct assoc_array_ptr *back_pointer,
+							int parent_slot)
 {
-	struct assoc_array_shortcut *shortcut, *new_s0;
-	struct assoc_array_node *node, *new_n0, *new_n1, *side;
-	struct assoc_array_ptr *ptr;
-	unsigned long dissimilarity, base_seg, blank;
-	size_t keylen;
-	bool have_meta;
-	int level, diff;
-	int slot, next_slot, free_slot, i, j;
-
-	node	= result->terminal_node.node;
-	level	= result->terminal_node.level;
-	edit->segment_cache[ASSOC_ARRAY_FAN_OUT] = result->terminal_node.slot;
+	if (!back_pointer)
+		return &array->root;
+	if (assoc_array_ptr_is_node(back_pointer))
+		return &assoc_array_ptr_to_node(back_pointer)->slots[parent_slot];
+	return &assoc_array_ptr_to_shortcut(back_pointer)->next_node;
+}
 
-	pr_devel("-->%s()\n", __func__);
+/*
+ * Insert into a terminal node without changing its shape, if that can be
+ * done: in place of a leaf with the same index key, or in a free slot.
+ */
+static bool assoc_array_insert_in_node(struct assoc_array_edit *edit,
+				       const struct assoc_array_ops *ops,
+				       const void *index_key,
+				       struct assoc_array_node *node)
+{
+	struct assoc_array_ptr *ptr;
+	int free_slot, i;
 
-	/* We arrived at a node which doesn't have an onward node or shortcut
-	 * pointer that we have to follow.  This means that (a) the leaf we
-	 * want must go here (either by insertion or replacement) or (b) we
-	 * need to split this node and insert in one of the fragments.
-	 */
 	free_slot = -1;
 
 	/* Firstly, we have to check the leaves in this node to see if there's
@@ -514,7 +513,6 @@ static bool assoc_array_insert_into_terminal_node(struct assoc_array_edit *edit,
 			pr_devel("replace in slot %d\n", i);
 			edit->leaf_p = &node->slots[i];
 			edit->dead_leaf = node->slots[i];
-			pr_devel("<--%s() = ok [replace]\n", __func__);
 			return true;
 		}
 	}
@@ -526,28 +524,28 @@ static bool assoc_array_insert_into_terminal_node(struct assoc_array_edit *edit,
 		pr_devel("insert in free slot %d\n", free_slot);
 		edit->leaf_p = &node->slots[free_slot];
 		edit->adjust_count_on = node;
-		pr_devel("<--%s() = ok [insert]\n", __func__);
 		return true;
 	}
 
-	/* The node has no spare slots - so we're either going to have to split
-	 * it or insert another node before it.
-	 *
-	 * Whatever, we're going to need at least two new nodes - so allocate
-	 * those now.  We may also need a new shortcut, but we deal with that
-	 * when we need it.
-	 */
-	new_n0 = kzalloc_obj(struct assoc_array_node);
-	if (!new_n0)
-		return false;
-	edit->new_meta[0] = assoc_array_node_to_ptr(new_n0);
-	new_n1 = kzalloc_obj(struct assoc_array_node);
-	if (!new_n1)
-		return false;
-	edit->new_meta[1] = assoc_array_node_to_ptr(new_n1);
+	return false;
+}
+
+/*
+ * Fill in the edit script's segment cache for a full node: the segment at
+ * this level of each leaf's index key, or 0xff for a metadata pointer.
+ * The new key's segment, in the last entry, is left to the caller.  Returns
+ * true if there are any metadata pointers.
+ */
+static bool assoc_array_fill_segment_cache(struct assoc_array_edit *edit,
+					   const struct assoc_array_ops *ops,
+					   const struct assoc_array_node *node,
+					   int level)
+{
+	struct assoc_array_ptr *ptr;
+	unsigned long base_seg;
+	bool have_meta;
+	int i;
 
-	/* We need to find out how similar the leaves are. */
-	pr_devel("no spare slots\n");
 	have_meta = false;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assoc_array_gc`
- 外部函数：`assoc_array_leaf_to_ptr`
- 外部函数：`assoc_array_node_to_ptr`
- 外部函数：`assoc_array_ptr_is_leaf`
- 外部函数：`assoc_array_ptr_is_meta`
- 外部函数：`assoc_array_ptr_is_node`
- 外部函数：`assoc_array_ptr_is_shortcut`
- 外部函数：`assoc_array_ptr_to_leaf`
- 外部函数：`assoc_array_ptr_to_node`
- 外部函数：`assoc_array_ptr_to_shortcut`
- 外部函数：`assoc_array_shortcut_to_ptr`
- 外部函数：`bool`
- 外部函数：`call_rcu`
- 外部函数：`compare_object`
- 外部函数：`container_of`
- 外部函数：`flex_array_size`
- 外部函数：`free_object`
- 外部函数：`get_key_chunk`
- 外部函数：`get_object_key_chunk`
- 外部函数：`here`
- 外部函数：`int`
- 外部函数：`iterator`
- 外部函数：`kfree`
- 外部函数：`kmalloc_flex`
- 外部函数：`kzalloc_flex`
- 外部函数：`kzalloc_obj`
- 外部函数：`leaf`
- 外部函数：`memcpy`
- 外部函数：`node`
- 外部函数：`nodes`
- 外部函数：`or`
- 外部函数：`pr_devel`
- 外部函数：`round_down`
- 外部函数：`round_up`
- 外部函数：`s`
- 外部函数：`smp_wmb`
- 外部函数：`struct_size`
- 外部函数：`that`
- 外部函数：`word`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`ASSOC_ARRAY_FAN_MASK`
- 大写宏：`ASSOC_ARRAY_FAN_OUT`
- 大写宏：`ASSOC_ARRAY_KEY_CHUNK_MASK`
- 大写宏：`ASSOC_ARRAY_KEY_CHUNK_SHIFT`
- 大写宏：`ASSOC_ARRAY_KEY_CHUNK_SIZE`
- 大写宏：`ASSOC_ARRAY_LEVEL_STEP`
- 大写宏：`ASSOC_ARRAY_LEVEL_STEP_MASK`
- 大写宏：`ASSOC_ARRAY_PTR_META_TYPE`
- 大写宏：`BUG`
- 大写宏：`BUG_ON`
- 大写宏：`ENOMEM`
- 大写宏：`ERR_PTR`
- 大写宏：`NULL`
- 大写宏：`RCU`
- 大写宏：`READ_ONCE`
- 大写宏：`ULONG_MAX`
- 外部类型：`Accesses`
- 外部类型：`Address`
- 外部类型：`All`
- 外部类型：`Ascend`
- 外部类型：`Check`
- 外部类型：`Clean`
- 外部类型：`Collect`
- 外部类型：`Create`
- 外部类型：`Descend`
- 外部类型：`Done`
- 外部类型：`Duplicate`
- 外部类型：`Eventually`
- 外部类型：`Excise`
- 外部类型：`Filter`
- 外部类型：`First`
- 外部类型：`Firstly`
- 外部类型：`Garbage`
- 外部类型：`Handle`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Invoke`
- 外部类型：`It`
- 外部类型：`Look`
- 外部类型：`Move`
- 外部类型：`Need`
- 外部类型：`Node`
- 外部类型：`Otherwise`
- 外部类型：`Private`
- 外部类型：`Script`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Trim`
- 外部类型：`Use`
- 外部类型：`We`
- 外部类型：`Whatever`
- 外部类型：`assoc_array`
- 外部类型：`assoc_array_edit`
- 外部类型：`assoc_array_node`
- 外部类型：`assoc_array_ops`
- 外部类型：`assoc_array_ptr`
- 外部类型：`assoc_array_shortcut`
- 外部类型：`rcu_head`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-53fcf34ce5` → 本草稿移入 `cases/defect/auto-linux-53fcf34ce5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
