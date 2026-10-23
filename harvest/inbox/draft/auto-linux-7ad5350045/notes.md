# auto-linux-7ad5350045

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#bf2325084583daaea91084b25069f121d5d8832b](https://github.com/torvalds/linux/commit/bf2325084583daaea91084b25069f121d5d8832b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 59 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bf2325084583daaea91084b25069f121d5d8832b (https://github.com/torvalds/linux/commit/bf2325084583daaea91084b25069f121d5d8832b)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 318；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bf2325084583daaea91084b25069f121d5d8832b Merge tag 'v7.4-p1' of git://git.kernel.org/pub/scm/linux/kernel/git/herbert/crypto-2.6 :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -56,6 +56,21 @@
  */
 #define get_driver_name(tfm_type, tfm) crypto_tfm_alg_driver_name(tfm_type ## _tfm(tfm))
 
+struct tcrypt_aead_request {
+	struct aead_request *req;
+	struct crypto_wait wait;
+};
+
+struct tcrypt_ahash_request {
+	struct ahash_request *req;
+	struct crypto_wait wait;
+};
+
+struct tcrypt_skcipher_request {
+	struct skcipher_request *req;
+	struct crypto_wait wait;
+};
+
 /*
  * Used by test_cipher_speed()
  */
@@ -129,18 +144,15 @@ static void sg_init_aead(struct scatterlist *sg, char *xbuf[XBUFSIZE],
 		sg_set_buf(&sg[k + 1], xbuf[k], rem);
 }
 
-static inline int do_one_aead_op(struct aead_request *req, int ret)
+static inline int do_one_aead_op(struct tcrypt_aead_request *req, int ret)
 {
-	struct crypto_wait *wait = req->base.data;
-
-	return crypto_wait_req(ret, wait);
+	return crypto_wait_req(ret, &req->wait);
 }
 
 struct test_mb_aead_data {
 	struct scatterlist sg[XBUFSIZE];
 	struct scatterlist sgout[XBUFSIZE];
-	struct aead_request *req;
-	struct crypto_wait wait;
+	struct tcrypt_aead_request req;
 	char *xbuf[XBUFSIZE];
 	char *xoutbuf[XBUFSIZE];
 	char *axbuf[XBUFSIZE];
@@ -154,14 +166,14 @@ static int do_mult_aead_op(struct test_mb_aead_data *data, int enc,
 	/* Fire up a bunch of concurrent requests */
 	for (i = 0; i < num_mb; i++) {
 		if (enc == ENCRYPT)
-			rc[i] = crypto_aead_encrypt(data[i].req);
+			rc[i] = crypto_aead_encrypt(data[i].req.req);
 		else
-			rc[i] = crypto_aead_decrypt(data[i].req);
+			rc[i] = crypto_aead_decrypt(data[i].req.req);
 	}
 
 	/* Wait for all requests to finish */
 	for (i = 0; i < num_mb; i++) {
-		rc[i] = crypto_wait_req(rc[i], &data[i].wait);
+		rc[i] = crypto_wait_req(rc[i], &data[i].req.wait);
 
 		if (rc[i]) {
 			pr_info("concurrent request %d error %d\n", i, rc[i]);
@@ -310,21 +322,21 @@ static void test_mb_aead_speed(const char *algo, int enc, int secs,
 		}
 
 	for (i = 0; i < num_mb; ++i) {
-		data[i].req = aead_request_alloc(tfm, GFP_KERNEL);
-		if (!data[i].req) {
+		data[i].req.req = aead_request_alloc(tfm, GFP_KERNEL);
+		if (!data[i].req.req) {
 			pr_err("alg: aead: Failed to allocate request for %s\n",
 			       algo);
 			while (i--)
-				aead_request_free(data[i].req);
+				aead_request_free(data[i].req.req);
 			goto out_free_xoutbuf;
 		}
 	}
 
 	for (i = 0; i < num_mb; ++i) {
-		crypto_init_wait(&data[i].wait);
-		aead_request_set_callback(data[i].req,
+		crypto_init_wait(&data[i].req.wait);
+		aead_request_set_callback(data[i].req.req,
 					  CRYPTO_TFM_REQ_MAY_BACKLOG,
-					  crypto_req_done, &data[i].wait);
+					  crypto_req_done, &data[i].req.wait);
 	}
 
 	pr_info("testing speed of multibuffer %s (%s) %s\n", algo,
@@ -386,16 +398,16 @@ static void test_mb_aead_speed(const char *algo, int enc, int secs,
 					     bs + (enc ? authsize : 0),
 					     assoc, aad_size);
 
-				aead_request_set_ad(cur->req, aad_size);
+				aead_request_set_ad(cur->req.req, aad_size);
 
 				if (!enc) {
 
-					aead_request_set_crypt(cur->req,
+					aead_request_set_crypt(cur->req.req,
 							       cur->sgout,
 							       cur->sg,
 							       bs, iv);
-					ret = crypto_aead_encrypt(cur->req);
-					ret = do_one_aead_op(cur->req, ret);
+					ret = crypto_aead_encrypt(cur->req.req);
+					ret = do_one_aead_op(&cur->req, ret);
 
 					if (ret) {
 						pr_err("calculating auth failed (%d)\n",
@@ -404,7 +416,7 @@ static void test_mb_aead_speed(const char *algo, int enc, int secs,
 					}
 				}
 
-				aead_request_set_crypt(cur->req, cur->sg,
+				aead_request_set_crypt(cur->req.req, cur->sg,
 						       cur->sgout, bs +
 						       (enc ? 0 : authsize),
 						       iv);
@@ -432,7 +444,7 @@ static void test_mb_aead_speed(const char *algo, int enc, int secs,
 
 out:
 	for (i = 0; i < num_mb; ++i)
-		aead_request_free(data[i].req);
+		aead_request_free(data[i].req.req);
 out_free_xoutbuf:
 	for (i = 0; i < num_mb; ++i)
 		testmgr_free_buf(data[i].xoutbuf);
@@ -450,19 +462,20 @@ static void test_mb_aead_speed(const char *algo, int enc
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_tfm`
- 外部函数：`aead_request_alloc`
- 外部函数：`aead_request_free`
- 外部函数：`aead_request_set_ad`
- 外部函数：`aead_request_set_callback`
- 外部函数：`aead_request_set_crypt`
- 外部函数：`crypto_aead_decrypt`
- 外部函数：`crypto_aead_encrypt`
- 外部函数：`crypto_init_wait`
- 外部函数：`crypto_tfm_alg_driver_name`
- 外部函数：`crypto_wait_req`
- 外部函数：`failed`
- 外部函数：`get_driver_name`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`s`
- 外部函数：`sg_set_buf`
- 外部函数：`test_cipher_speed`
- 大写宏：`CRYPTO_TFM_REQ_MAY_BACKLOG`
- 大写宏：`ENCRYPT`
- 大写宏：`GFP_KERNEL`
- 大写宏：`XBUFSIZE`
- 外部类型：`Failed`
- 外部类型：`Fire`
- 外部类型：`Used`
- 外部类型：`Wait`
- 外部类型：`aead_request`
- 外部类型：`crypto_wait`
- 外部类型：`scatterlist`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7ad5350045` → 本草稿移入 `cases/defect/auto-linux-7ad5350045/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
