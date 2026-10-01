# auto-redis-9908d815d6

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15111 (https://github.com/redis/redis/pull/15111)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 265；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -48,6 +48,195 @@ static const double powers_of_ten[] = {
     1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
 };
 
+/* ----------------------------------------------------------------------------
+ * Eisel-Lemire algorithm — extended-precision powers of five.
+ *
+ * The table below maps from decimal scaling (10^q) to a 128-bit binary
+ * approximation. Since 10^q = 2^q * 5^q and the 2^q factor is exact in
+ * binary, only 5^q affects the binary significand — so we precompute
+ * 5^q rounded toward 1 to 128 bits. Used by `compute_float()` to avoid
+ * any iterative rounding in the widened (mantissa > 2^53) range.
+ *
+ * Pulled verbatim from fast_float by Daniel Lemire & Joao Paulo Magalhaes
+ * (MIT-licensed, https://github.com/fastfloat/fast_float — fast_table.h).
+ *
+ * Range: 5^-342 ... 5^308 — covers every value that can produce a finite
+ * non-zero double from a 64-bit decimal mantissa. 651 entries, each stored
+ * as { high64, low64 } pairs (1302 uint64_t total).
+ * ---------------------------------------------------------------------------- */
+
+#define EISEL_LEMIRE_SMALLEST_POWER_OF_FIVE -342
+#define EISEL_LEMIRE_LARGEST_POWER_OF_FIVE   308
+#define EISEL_LEMIRE_NUMBER_OF_ENTRIES (2 * (EISEL_LEMIRE_LARGEST_POWER_OF_FIVE - \
+                                             EISEL_LEMIRE_SMALLEST_POWER_OF_FIVE + 1))
+
+static const uint64_t power_of_five_128[EISEL_LEMIRE_NUMBER_OF_ENTRIES] = {
+    0xeef453d6923bd65a, 0x113faa2906a13b3f, 0x9558b4661b6565f8, 0x4ac7ca59a424c507, 0xbaaee17fa23ebf76, 0x5d79bcf00d2df649, 0xe95a99df8ace6f53, 0xf4d82c2c107973dc,
+    0x91d8a02bb6c10594, 0x79071b9b8a4be869, 0xb64ec836a47146f9, 0x9748e2826cdee284, 0xe3e27a444d8d98b7, 0xfd1b1b2308169b25, 0x8e6d8c6ab0787f72, 0xfe30f0f5e50e20f7,
+    0xb208ef855c969f4f, 0xbdbd2d335e51a935, 0xde8b2b66b3bc4723, 0xad2c788035e61382, 0x8b16fb203055ac76, 0x4c3bcb5021afcc31, 0xaddcb9e83c6b1793, 0xdf4abe242a1bbf3d,
+    0xd953e8624b85dd78, 0xd71d6dad34a2af0d, 0x87d4713d6f33aa6b, 0x8672648c40e5ad68, 0xa9c98d8ccb009506, 0x680efdaf511f18c2, 0xd43bf0effdc0ba48, 0x212bd1b2566def2,
+    0x84a57695fe98746d, 0x14bb630f7604b57, 0xa5ced43b7e3e9188, 0x419ea3bd35385e2d, 0xcf42894a5dce35ea, 0x52064cac828675b9, 0x818995ce7aa0e1b2, 0x7343efebd1940993,
+    0xa1ebfb4219491a1f, 0x1014ebe6c5f90bf8, 0xca66fa129f9b60a6, 0xd41a26e077774ef6, 0xfd00b897478238d0, 0x8920b098955522b4, 0x9e20735e8cb16382, 0x55b46e5f5d5535b0,
+    0xc5a890362fddbc62, 0xeb2189f734aa831d, 0xf712b443bbd52b7b, 0xa5e9ec7501d523e4, 0x9a6bb0aa55653b2d, 0x47b233c92125366e, 0xc1069cd4eabe89f8, 0x999ec0bb696e840a,
+    0xf148440a256e2c76, 0xc00670ea43ca250d, 0x96cd2a865764dbca, 0x380406926a5e5728, 0xbc807527ed3e12bc, 0xc605083704f5ecf2, 0xeba09271e88d976b, 0xf7864a44c633682e,
+    0x93445b8731587ea3, 0x7ab3ee6afbe0211d, 0xb8157268fdae9e4c, 0x5960ea05bad82964, 0xe61acf033d1a45df, 0x6fb92487298e33bd, 0x8fd0c16206306bab, 0xa5d3b6d479f8e056,
+    0xb3c4f1ba87bc8696, 0x8f48a4899877186c, 0xe0b62e2929aba83c, 0x331acdabfe94de87, 0x8c71dcd9ba0b4925, 0x9ff0c08b7f1d0b14, 0xaf8e5410288e1b6f, 0x7ecf0ae5ee44dd9,
+    0xdb71e91432b1a24a, 0xc9e82cd9f69d6150, 0x892731ac9faf056e, 0xbe311c083a225cd2, 0xab70fe17c79ac6ca, 0x6dbd630a48aaf406, 0xd64d3d9db981787d, 0x92cbbccdad5b108,
+    0x85f0468293f0eb4e, 0x25bbf56008c58ea5, 0xa76c582338ed2621, 0xaf2af2b80af6f24e, 0xd1476e2c07286faa, 0x1af5af660db4aee1, 0x82cca4db847945ca, 0x50d98d9fc890ed4d,
+    0xa37fce126597973c, 0xe50ff107bab528a0, 0xcc5fc196fefd7d0c, 0x1e53ed49a96272c8, 0xff77b1fcbebcdc4f, 0x25e8e89c13bb0f7a, 0x9faacf3df73609b1, 0x77b191618c54e9ac,
+    0xc795830d75038c1d, 0xd59df5b9ef6a2417, 0xf97ae3d0d2446f25, 0x4b0573286b44ad1d, 0x9becce62836ac577, 0x4ee367f9430aec32, 0xc2e801fb244576d5, 0x229c41f793cda73f,
+    0xf3a20279ed56d48a, 0x6b43527578c1110f, 0x9845418c345644d6, 0x830a13896b78aaa9, 0xbe5691ef416bd60c, 0x23cc986bc656d553, 0xedec366b11c6cb8f, 0x2cbfbe86b7ec8aa8,
+    0x94b3a202eb1c3f39, 0x7bf7d71432f3d6a9, 0xb9e08a83a5e34f07, 0xdaf5ccd93fb0cc53
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-9908d815d6` → 本草稿移入 `cases/defect/auto-redis-9908d815d6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
