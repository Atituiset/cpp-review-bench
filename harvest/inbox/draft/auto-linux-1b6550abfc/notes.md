# auto-linux-1b6550abfc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0fcb05b72344f40806cbe810de8872f1698bf053](https://github.com/torvalds/linux/commit/0fcb05b72344f40806cbe810de8872f1698bf053) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0fcb05b72344f40806cbe810de8872f1698bf053 (https://github.com/torvalds/linux/commit/0fcb05b72344f40806cbe810de8872f1698bf053)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0fcb05b72344f40806cbe810de8872f1698bf053 Merge tag 'libcrypto-fixes-for-linus' of git://git.kernel.org/pub/scm/linux/kernel/git/ebiggers/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -55,16 +55,17 @@ static int poly1305_suite_init(struct kunit_suite *suite)
  * - Using an all-one-bits r_key tests the key clamping.
  * - Using an all-one-bits s_key tests carries in implementations of the
  *   addition mod 2**128 during finalization.
- * - Using all-one-bits message, and to a lesser extent r_key, tends to maximize
- *   any intermediate accumulator values.  This increases the chance of
- *   detecting bugs that occur only in rare cases where the accumulator values
- *   get very large, for example the bug fixed by commit 678cce4019d746da
- *   ("crypto: x86/poly1305 - fix overflow during partial reduction").
+ * - Using all-one-bits message and r_key results in large values for the
+ *   internal products of r_key by (accumulator + message block).  This
+ *   increases the chance of detecting bugs that occur only in rare cases where
+ *   the sum of these internal products is very large, for example the bug fixed
+ *   by commit 678cce4019d7 ("crypto: x86/poly1305 - fix overflow during partial
+ *   reduction").
  *
- * Accumulator overflow bugs may be specific to particular update lengths (in
- * blocks) and/or particular values of the previous acculumator.  Note that the
- * accumulator starts at 0 which gives the lowest chance of an overflow.  Thus,
- * a single all-one-bits test vector may be insufficient.
+ * Bugs with the handling of large internal product sums may be specific to
+ * particular update lengths (in blocks) and/or require that the accumulator
+ * also have a large value.  Note that the accumulator starts at 0.  Thus, a
+ * single all-one-bits test vector may be insufficient.
  *
  * Considering that, do the following test: continuously update a single
  * Poly1305 context with all-one-bits data of varying lengths (0, 16, 32, ...,
@@ -141,10 +142,88 @@ static void test_poly1305_reduction_edge_cases(struct kunit *test)
 	}
 }
 
+/*
+ * Test that Poly1305 MACs are computed correctly when r_key=1, s_key=0, and the
+ * message consists of all-ones blocks.
+ *
+ * With r_key=1 and s_key=0, Poly1305 degrades to the sum of the padded blocks
+ * mod 2**130 - 5, then reduced mod 2**128.  A padded all-ones block maps to
+ * 2**128 + (2**128 - 1) = 2**129 - 1.  Two such blocks sum to 2**130 - 2, which
+ * is congruent to 3 mod 2**130 - 5.  Thus, the expected MAC is just 3 *
+ * floor(nblocks / 2), minus 1 if nblocks is odd, then reduced mod 2**128.  If
+ * nblocks == 1 this is 2**128 - 1, otherwise it's just a small integer.
+ *
+ * Inside the Poly1305 implementation in this case, the multiplication does
+ * nothing and each block just adds 2**129 - 1 to the accumulator.  Without the
+ * multiplication to mix things up, this results in some interesting values
+ * being reached where the limbs are at or near their maximum values, even after
+ * (lazy) reduction.  This can reproduce bugs that happen only in such cases.
+ *
+ * For example, this test case reproduces the bug fixed by commit 24f07e8c07a0
+ * ("lib/crypto: arm64: Fix lost Poly1305 carry when resuming NEON state").
+ *
+ * However, reaching that bug required two consecutive updates: one with >= 8
+ * blocks to cause NEON code to be used and leave the accumulator in base 2**26
+ * with its limbs in a certain pattern, then one with an odd number of blocks >=
+ * 9 to cause NEON to be used again, but first processing one block using scalar
+ * code as a special case, triggering a conversion to base 2**64 where the carry
+ * into the 2**128 bit was lost.  To cover this and other similar edge cases
+ * that may exist in other Poly1305 implementations, just test all possible
+ * block-aligned splits between three poly1305_update() calls.
+ */
+static void test_poly1305_split_update_carry(struct kunit *test)
+{
+	static const u8 key[POLY1305_KEY_SIZE] = { 1 }; /* r_key=1, s_key=0 */
+	/*
+	 * Use max_nblocks=66 so that it's a bit more than twice the AVX-512
+	 * threshold of 32 blocks.
+	 */
+	const int max_nblocks = 66;
+	u8 *data = alloc_buf
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`alloc_buf`
- 外部函数：`memset`
- 外部函数：`poly1305_final`
- 外部函数：`poly1305_init`
- 外部函数：`poly1305_update`
- 外部函数：`test_poly1305_allones_keys_and_message`
- 大写宏：`HASH_KUNIT_CASES`
- 大写宏：`KUNIT_ASSERT_MEMEQ`
- 大写宏：`KUNIT_CASE`
- 大写宏：`MAC`
- 大写宏：`POLY1305_BLOCK_SIZE`
- 大写宏：`POLY1305_DIGEST_SIZE`
- 大写宏：`POLY1305_KEY_SIZE`
- 外部类型：`Accumulator`
- 外部类型：`Assuming`
- 外部类型：`Compute`
- 外部类型：`Considering`
- 外部类型：`If`
- 外部类型：`Note`
- 外部类型：`Poly1305`
- 外部类型：`Set`
- 外部类型：`This`
- 外部类型：`Thus`
- 外部类型：`Using`
- 外部类型：`kunit`
- 外部类型：`kunit_case`
- 外部类型：`poly1305_desc_ctx`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-1b6550abfc` → 本草稿移入 `cases/defect/auto-linux-1b6550abfc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
