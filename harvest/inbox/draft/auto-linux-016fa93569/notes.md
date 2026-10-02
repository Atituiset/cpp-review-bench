# auto-linux-016fa93569

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#bd35955b089ad881f57a5a2619687c804d057fec](https://github.com/torvalds/linux/commit/bd35955b089ad881f57a5a2619687c804d057fec) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 38 |
| 编译错误数（gcc syntax-only） | 43（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bd35955b089ad881f57a5a2619687c804d057fec (https://github.com/torvalds/linux/commit/bd35955b089ad881f57a5a2619687c804d057fec)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bd35955b089ad881f57a5a2619687c804d057fec Merge tag 'libcrypto-fixes-for-linus' of git://git.kernel.org/pub/scm/linux/kernel/git/ebiggers/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -637,7 +637,18 @@ static struct skcipher_alg skcipher_algs[] = {
 		.decrypt = crypto_aes_cbc_decrypt,
 	},
 #endif
-#if IS_ENABLED(CONFIG_CRYPTO_CTS)
+	/*
+	 * Don't register library-based "cts(cbc(aes))" on architectures where
+	 * it might block a "better" implementation from being instantiated via
+	 * the "cts" template.  These exclusions are temporary and will go away
+	 * as the arch-optimized AES code is migrated into the library.
+	 */
+#if IS_ENABLED(CONFIG_CRYPTO_CTS) && \
+	!(IS_ENABLED(CONFIG_ARM) || \
+	  IS_ENABLED(CONFIG_ARM64) || \
+	  IS_ENABLED(CONFIG_PPC) || \
+	  IS_ENABLED(CONFIG_S390) || \
+	  IS_ENABLED(CONFIG_SPARC))
 	{
 		.base.cra_name = "cts(cbc(aes))",
 		.base.cra_driver_name = "cts-cbc-aes-lib",
@@ -687,7 +698,13 @@ static struct skcipher_alg skcipher_algs[] = {
 		.decrypt = crypto_aes_xctr_crypt,
 	},
 #endif
-#if IS_ENABLED(CONFIG_CRYPTO_XTS)
+	/*
+	 * Don't register library-based "xts(aes)" on architectures where it
+	 * might block a "better" implementation from being instantiated via the
+	 * "xts" template.  This exclusion is temporary and will go away when
+	 * the library AES-XTS is optimized for SPARC.
+	 */
+#if IS_ENABLED(CONFIG_CRYPTO_XTS) && !IS_ENABLED(CONFIG_SPARC)
 	{
 		.base.cra_name = "xts(aes)",
 		.base.cra_driver_name = "xts-aes-lib",
@@ -980,7 +997,20 @@ static __maybe_unused int crypto_aes_ccm_decrypt(struct aead_request *req)
 }
 
 static struct aead_alg aead_algs[] = {
-#if IS_ENABLED(CONFIG_CRYPTO_GCM)
+	/*
+	 * Don't register library-based "gcm(aes)" and "rfc4106(gcm(aes))" on
+	 * architectures where they might block a "better" implementation from
+	 * being instantiated via the "gcm" and "rfc4106" templates.  These
+	 * exclusions are temporary and will go away as the arch-optimized AES
+	 * code is migrated into the library.
+	 */
+#if IS_ENABLED(CONFIG_CRYPTO_GCM) && \
+	!(IS_ENABLED(CONFIG_ARM) || \
+	  IS_ENABLED(CONFIG_ARM64) || \
+	  IS_ENABLED(CONFIG_PPC) || \
+	  IS_ENABLED(CONFIG_RISCV) || \
+	  IS_ENABLED(CONFIG_S390) || \
+	  IS_ENABLED(CONFIG_SPARC))
 	{
 		.base.cra_name = "gcm(aes)",
 		.base.cra_driver_name = "gcm-aes-lib",
@@ -1012,7 +1042,20 @@ static struct aead_alg aead_algs[] = {
 		.chunksize = AES_BLOCK_SIZE,
 	},
 #endif /* CONFIG_CRYPTO_GCM */
-#if IS_ENABLED(CONFIG_CRYPTO_CCM)
+	/*
+	 * Don't register library-based "ccm(aes)" on architectures where it
+	 * might block a "better" implementation from being instantiated via the
+	 * "ccm" template.  These exclusions are temporary and will go away as
+	 * the arch-optimized AES code is migrated into the library.
+	 */
+#if IS_ENABLED(CONFIG_CRYPTO_CCM) && \
+	!(IS_ENABLED(CONFIG_ARM) || \
+	  IS_ENABLED(CONFIG_ARM64) || \
+	  IS_ENABLED(CONFIG_PPC) || \
+	  IS_ENABLED(CONFIG_RISCV) || \
+	  IS_ENABLED(CONFIG_S390) || \
+	  IS_ENABLED(CONFIG_SPARC) || \
+	  IS_ENABLED(CONFIG_X86))
 	{
 		.base.cra_name = "ccm(aes)",
 		.base.cra_driver_name = "ccm-aes-lib",
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__aligned`
- 外部函数：`__alignof__`
- 外部函数：`cbc`
- 外部函数：`ccm`
- 外部函数：`crypt_func`
- 外部函数：`crypto_skcipher_ctx`
- 外部函数：`crypto_skcipher_reqtfm`
- 外部函数：`cts`
- 外部函数：`gcm`
- 外部函数：`memcpy_from_scatterwalk`
- 外部函数：`memcpy_to_scatterwalk`
- 外部函数：`memzero_explicit`
- 外部函数：`min`
- 外部函数：`offset_in_page`
- 外部函数：`round_down`
- 外部函数：`scatterwalk_advance`
- 外部函数：`scatterwalk_clamp`
- 外部函数：`scatterwalk_done_dst`
- 外部函数：`scatterwalk_map`
- 外部函数：`scatterwalk_start_at_pos`
- 外部函数：`scatterwalk_unmap`
- 外部函数：`sg_page`
- 外部函数：`unlikely`
- 外部函数：`xts`
- 大写宏：`AES_BLOCK_SIZE`
- 大写宏：`CONFIG_CRYPTO_CCM`
- 大写宏：`CONFIG_CRYPTO_CTS`
- 大写宏：`CONFIG_CRYPTO_GCM`
- 大写宏：`CONFIG_CRYPTO_XTS`
- 大写宏：`CONFIG_HIGHMEM`
- 大写宏：`EINVAL`
- 大写宏：`IS_ENABLED`
- 大写宏：`PAGE_SIZE`
- 外部类型：`aead_alg`
- 外部类型：`aes_enckey`
- 外部类型：`aes_key`
- 外部类型：`scatter_walk`
- 外部类型：`skcipher_request`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-016fa93569` → 本草稿移入 `cases/defect/auto-linux-016fa93569/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
