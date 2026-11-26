# auto-linux-06534d390d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc](https://github.com/torvalds/linux/commit/2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 83（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc (https://github.com/torvalds/linux/commit/2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 99；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc Merge tag 'probes-fixes-v7.4-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -96,7 +96,8 @@ fetch_store_symstring(unsigned long addr, void *dest, void *base)
 		return -ENOMEM;
 
 	__dest = get_loc_data(dest, base);
-	len = sprint_symbol(namebuf, addr);
+	/* Do not dereference NULL pointers */
+	len = addr ? sprint_symbol(namebuf, addr) : -1;
 	if (len < 0) {
 		*(u32 *)dest = make_data_loc(0, __dest - base);
 		return 0;
@@ -133,39 +134,92 @@ process_common_fetch_insn(struct fetch_insn *code, unsigned long *val)
 	return 0;
 }
 
+/* On failure, zero out the destination */
+static nokprobe_inline int
+do_probe_mem_read_user(void *dest, void *src, size_t size)
+{
+	int ret;
+
+	/* Do not dereference NULL pointers */
+	ret = src ? probe_mem_read_user(dest, src, size) : -1;
+	if (ret < 0)
+		memset(dest, 0, size);
+	return ret;
+}
+
+/* On failure, zero out the destination */
+static nokprobe_inline int
+do_probe_mem_read(void *dest, void *src, size_t size)
+{
+	int ret;
+
+	/* Do not dereference NULL pointers */
+	ret = src ? probe_mem_read(dest, src, size) : -1;
+	if (ret < 0)
+		memset(dest, 0, size);
+	return ret;
+}
+
+static nokprobe_inline int
+do_fetch_store_string(unsigned long addr, void *dest, void *base)
+{
+	/* NULL addr gets zero size string */
+	if (!addr) {
+		void *__dest = get_loc_data(dest, base);
+
+		*(u32 *)dest = make_data_loc(0, __dest - base);
+		return 0;
+	}
+	return fetch_store_string(addr, dest, base);
+}
+
+static nokprobe_inline int
+do_fetch_store_string_user(unsigned long addr, void *dest, void *base)
+{
+	/* NULL addr gets zero size string */
+	if (!addr) {
+		void *__dest = get_loc_data(dest, base);
+
+		*(u32 *)dest = make_data_loc(0, __dest - base);
+		return 0;
+	}
+	return fetch_store_string_user(addr, dest, base);
+}
+
 /* From the 2nd stage, routine is same */
 static nokprobe_inline int
 process_fetch_insn_bottom(struct fetch_insn *code, unsigned long val,
 			   void *dest, void *base)
 {
 	struct fetch_insn *s3 = NULL;
-	int total = 0, ret = 0, i = 0;
+	int total = 0, ret = 0, i = 0, offset;
 	u32 loc = 0;
 	unsigned long lval, llval = val;
 
 stage2:
 	/* 2nd stage: dereference memory if needed */
 	do {
 		lval = val;
+		/* Prevent dereferencing NULL val pointer */
+		offset = val ? code->offset : 0;
 		switch (code->op) {
 		case FETCH_OP_DEREF:
-			ret = probe_mem_read(&val, (void *)val + code->offset,
-					     sizeof(val));
+			do_probe_mem_read(&val, (void *)val + offset,
+					  sizeof(val));
 			break;
 		case FETCH_OP_UDEREF:
-			ret = probe_mem_read_user(&val,
-				 (void *)val + code->offset, sizeof(val));
+			do_probe_mem_read_user(&val, (void *)val + offset,
+					       sizeof(val));
 			break;
 		case FETCH_OP_CPU_PTR:
-			val = (unsigned long)this_cpu_ptr((void __percpu *)val);
-			ret = 0;
+			/* Do not dereference NULL pointers */
+			if (val)
+				val = (unsigned long)this_cpu_ptr((void __percpu *)val);
 			break;
 		default:
 			lval = llval;
 			goto out;
 		}
-		if (ret)
-			return ret;
 		llval = lval;
 		code++;
 	} while (1);
@@ -175,45 +229,52 @@ process_fetch_insn_bottom(struct fetch_insn *code, unsigned long val,
 stage3:
 	/* 3rd stage: store value to buffer */
 	if (unlikely(!dest)) {
+		offset = code->offset;
 		switch (code->op) {
 		case FETCH_OP_ST_STRING:
-			ret = fetch_store_strlen(val + code->offset);
+			ret = val ? fetch_store_strlen(val + offset) : 0;
 			code++;
 			goto array;
 		case FETCH_OP_ST_USTRING:
-			ret = fetch_store_strlen_user(val + code->offset);
+			ret = val ? fetch_store_strlen_user(val + offset) : 0;
 			code++;
 			goto array;
 		case FETCH_OP_ST_SYMSTR:
-			ret = fetch_store_symstrlen(val + code->offset);
+			ret = val ? fetch_store_symstrlen(val + offset) : 0;
 			code++;
 			goto array;
 		default:
 			return -EILSEQ;
 		}
 	}
 
+	/*
+	 * Once val is zero, it is pointing to NULL. Any new dereference is
+	 * also treated as a NULL pointer.
+	 */
+	offset = val ? code->offset : 0;
+
 	switch (code->op) {
 	case FETCH_OP_ST_RAW:
 		fetch_store_raw(val, code, dest);
 		break;
 	case FETCH_OP_ST_MEM
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fetch_store_string`
- 外部函数：`fetch_store_string_user`
- 外部函数：`fetch_store_strlen`
- 外部函数：`fetch_store_strlen_user`
- 外部函数：`get_loc_data`
- 外部函数：`get_loc_len`
- 外部函数：`make_data_loc`
- 外部函数：`memcpy`
- 外部函数：`min`
- 外部函数：`probe_mem_read`
- 外部函数：`probe_mem_read_user`
- 外部函数：`sprint_symbol`
- 外部函数：`this_cpu_ptr`
- 外部函数：`unlikely`
- 外部函数：`update_data_loc`
- 大写宏：`EILSEQ`
- 大写宏：`ENOMEM`
- 大写宏：`FETCH_OP_CPU_PTR`
- 大写宏：`FETCH_OP_DEREF`
- 大写宏：`FETCH_OP_ST_MEM`
- 大写宏：`FETCH_OP_ST_RAW`
- 大写宏：`FETCH_OP_ST_STRING`
- 大写宏：`FETCH_OP_ST_SYMSTR`
- 大写宏：`FETCH_OP_ST_UMEM`
- 大写宏：`FETCH_OP_ST_USTRING`
- 大写宏：`FETCH_OP_UDEREF`
- 大写宏：`KSYM_SYMBOL_LEN`
- 大写宏：`NULL`
- 大写宏：`TBD`
- 外部类型：`From`
- 外部类型：`fetch_insn`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-06534d390d` → 本草稿移入 `cases/defect/auto-linux-06534d390d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
