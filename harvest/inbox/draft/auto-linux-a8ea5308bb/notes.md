# auto-linux-a8ea5308bb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#22430ae5d90ab288b0ee2ad99ae941f4a666b694](https://github.com/torvalds/linux/commit/22430ae5d90ab288b0ee2ad99ae941f4a666b694) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #22430ae5d90ab288b0ee2ad99ae941f4a666b694 (https://github.com/torvalds/linux/commit/22430ae5d90ab288b0ee2ad99ae941f4a666b694)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 105；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 22430ae5d90ab288b0ee2ad99ae941f4a666b694 Merge tag 'printk-for-7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/printk/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -62,14 +62,50 @@ static void braille_write(u16 *buf)
 {
 	static u16 lastwrite[WIDTH];
 	unsigned char data[1 + 1 + 2*WIDTH + 2 + 1], csum = 0, *c;
+	struct nbcon_write_context wctxt = { };
+	unsigned long flags;
 	u16 out;
 	int i;
 
 	if (!braille_co)
 		return;
 
+	/*
+	 * Braille console is not registered in console_list. Instead, it
+	 * is integrated with VT and shows what appears on the graphical
+	 * console under console_lock(). From this POV it is a legacy
+	 * console. But it calls serial console driver which might be
+	 * converted to the NBCON API. It is similar to
+	 * nbcon_legacy_emit_next_record() except that we should try
+	 * harder to get the lock. Otherwise, the Braille device won't show
+	 * everything what is displayed on the terminal.
+	 *
+	 * In short, simulate the original locking using NBCON API.
+	 */
+	if (braille_co->flags & CON_NBCON) {
+		if (panic_on_this_cpu()) {
+			/*
+			 * This should be good enough in practice. Most/all
+			 * serial console drivers have the atomic callback.
+			 */
+			if (!braille_co->write_atomic)
+				return;
+
+			local_irq_save(flags);
+			/* NBCON API strictly requires the ownership. */
+			if (!nbcon_braille_try_acquire(braille_co, &wctxt)) {
+				local_irq_restore(flags);
+				return;
+			}
+		} else {
+			braille_co->device_lock(braille_co, &flags);
+			while (!nbcon_braille_try_acquire(braille_co, &wctxt))
+				cpu_relax();
+		}
+	}
+
 	if (!memcmp(lastwrite, buf, WIDTH * sizeof(*buf)))
-		return;
+		goto unlock_nbcon;
 	memcpy(lastwrite, buf, WIDTH * sizeof(*buf));
 
 #define SOH 1
@@ -102,7 +138,24 @@ static void braille_write(u16 *buf)
 	*c++ = csum;
 	*c++ = ETX;
 
-	braille_co->write(braille_co, data, c - data);
+	if (braille_co->flags & CON_NBCON) {
+		nbcon_write_context_set_buf(&wctxt, (char *)data, c - data);
+		if (panic_on_this_cpu())
+			braille_co->write_atomic(braille_co, &wctxt);
+		else
+			braille_co->write_thread(braille_co, &wctxt);
+	} else {
+		braille_co->write(braille_co, data, c - data);
+	}
+
+unlock_nbcon:
+	if (braille_co->flags & CON_NBCON) {
+		nbcon_braille_release(&wctxt);
+		if (panic_on_this_cpu())
+			local_irq_restore(flags);
+		else
+			braille_co->device_unlock(braille_co, flags);
+	}
 }
 
 /* Follow the VC cursor*/
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`write`
- 外部类型：`Follow`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a8ea5308bb` → 本草稿移入 `cases/defect/auto-linux-a8ea5308bb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
