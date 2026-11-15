# auto-linux-ee87c1279b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#aacb7797504183a90d66b19b1efb555cfe9b3eec](https://github.com/torvalds/linux/commit/aacb7797504183a90d66b19b1efb555cfe9b3eec) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 33 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #aacb7797504183a90d66b19b1efb555cfe9b3eec (https://github.com/torvalds/linux/commit/aacb7797504183a90d66b19b1efb555cfe9b3eec)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1082；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT aacb7797504183a90d66b19b1efb555cfe9b3eec Merge tag 'ovl-fixes-7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/overlayfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -27,6 +27,7 @@ struct ovl_lookup_data {
 	bool last;
 	char *redirect;
 	char *upperredirect;
+	char *lowerdata_redirect;
 	int metacopy;
 	/* Referring to last redirect xattr */
 	bool absolute_redirect;
@@ -39,13 +40,12 @@ static int ovl_check_redirect(const struct path *path, struct ovl_lookup_data *d
 	char *buf;
 	struct ovl_fs *ofs = OVL_FS(d->sb);
 
-	d->absolute_redirect = false;
 	buf = ovl_get_redirect_xattr(ofs, path, prelen + strlen(post));
 	if (IS_ERR_OR_NULL(buf))
 		return PTR_ERR(buf);
 
-	if (buf[0] == '/') {
-		d->absolute_redirect = true;
+	d->absolute_redirect = buf[0] == '/';
+	if (d->absolute_redirect) {
 		/*
 		 * One of the ancestor path elements in an absolute path
 		 * lookup in ovl_lookup_layer() could have been opaque and
@@ -1047,6 +1047,14 @@ static int ovl_maybe_lookup_lowerdata(struct dentry *dentry)
 out_err:
 	pr_warn_ratelimited("lazy lowerdata lookup failed (%pd2, err=%i)\n",
 			    dentry, err);
+	/*
+	 * The data of a metacopy file is missing or is not a regular file in
+	 * the data-only layers: the layers are corrupted.  Do not return the
+	 * error of the lookup (e.g. ENOENT) to the caller, it would make the
+	 * file look like it does not exist.
+	 */
+	if (err != -ENOMEM)
+		err = -EIO;
 	goto out;
 }
 
@@ -1079,7 +1087,8 @@ static bool ovl_check_follow_redirect(struct ovl_lookup_data *d)
 		pr_warn_ratelimited("refusing to follow metacopy origin for (%pd2)\n", d->dentry);
 		return false;
 	}
-	if ((d->redirect || d->upperredirect) && !ovl_redirect_follow(ofs)) {
+	/* Check follow to lower layer, redirects to lowerdata are not checked here */
+	if ((d->redirect || d->upperredirect) && !ovl_redirect_follow_lower(ofs)) {
 		pr_warn_ratelimited("refusing to follow redirect for (%pd2)\n", d->dentry);
 		return false;
 	}
@@ -1097,13 +1106,26 @@ struct ovl_lookup_ctx {
 	unsigned int ctr;
 };
 
+/*
+ * Without metacopy=on, the absolute redirect of a lower metacopy file is
+ * followed only into the data-only layers.
+ */
+static bool ovl_is_lowerdata_redirect(struct ovl_lookup_ctx *ctx,
+				      struct ovl_lookup_data *d)
+{
+	struct ovl_fs *ofs = OVL_FS(d->sb);
+
+	return ofs->numdatalayer && !ofs->config.metacopy && ctx->ctr &&
+	       d->metacopy && d->absolute_redirect;
+}
+
 static int ovl_lookup_layers(struct ovl_lookup_ctx *ctx, struct ovl_lookup_data *d)
 {
 	struct dentry *dentry = ctx->dentry;
 	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
 	struct ovl_entry *poe = OVL_E(dentry->d_parent);
 	struct ovl_entry *roe = OVL_E(dentry->d_sb->s_root);
-	bool check_redirect = (ovl_redirect_follow(ofs) || ofs->numdatalayer);
+	bool check_redirect = ovl_redirect_follow(ofs);
 	struct dentry *upperdir;
 	struct dentry *this;
 	struct dentry *origin = NULL;
@@ -1234,6 +1256,10 @@ static int ovl_lookup_layers(struct ovl_lookup_ctx *ctx, struct ovl_lookup_data
 			ctx->ctr++;
 		}
 
+		/* Lookup of lowerdata in the data-only layers only */
+		if (ovl_is_lowerdata_redirect(ctx, d))
+			d->stop = true;
+
 		if (d->stop)
 			break;
 
@@ -1251,6 +1277,8 @@ static int ovl_lookup_layers(struct ovl_lookup_ctx *ctx, struct ovl_lookup_data
 	if (d->metacopy && ctx->ctr && ofs->numdatalayer && d->absolute_redirect) {
 		d->metacopy = 0;
 		ctx->ctr++;
+		d->lowerdata_redirect = d->redirect;
+		d->redirect = NULL;
 	} else if (!ovl_check_follow_redirect(d)) {
 		err = -EPERM;
 		return err;
@@ -1367,24 +1395,32 @@ static int ovl_lookup_layers(struct ovl_lookup_ctx *ctx, struct ovl_lookup_data
 		};
 
 		/* Store lowerdata redirect for lazy lookup */
-		if (ctx->ctr > 1 && !d->is_dir && !ctx->stack[ctx->ctr - 1].dentry) {
-			oip.lowerdata_redirect = d->redirect;
-			d->redirect = NULL;
-		}
+		if (d->lowerdata_redirect)
+			oip.lowerdata_redirect = d->lowerdata_redirect;
 
 		inode = ovl_get_inode(dentry->d_sb, &oip);
 		if (IS_ERR(inode))
 			return PTR_ERR(inode);
 
+		d->lowerdata_redirect = NULL;
+
 		ctx->inode = inode;
 		if (ctx->upperdentry && !uppermetacopy)
 			ovl_set_flag(OVL_UPPERDAT
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dput`
- 外部函数：`failed`
- 外部函数：`kfree`
- 外部函数：`ovl_dentry_init_reval`
- 外部函数：`ovl_free_entry`
- 外部函数：`ovl_get_inode`
- 外部函数：`ovl_get_redirect_xattr`
- 外部函数：`ovl_lookup_layer`
- 外部函数：`ovl_redirect_follow`
- 外部函数：`ovl_set_flag`
- 外部函数：`ovl_stack_free`
- 外部函数：`pr_warn_ratelimited`
- 外部函数：`strlen`
- 大写宏：`EPERM`
- 大写宏：`IS_ERR`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`NULL`
- 大写宏：`OVL_E`
- 大写宏：`OVL_FS`
- 大写宏：`OVL_HAS_DIGEST`
- 大写宏：`OVL_I_E`
- 大写宏：`OVL_METACOPY_MIN_SIZE`
- 大写宏：`OVL_UPPERDATA`
- 大写宏：`PTR_ERR`
- 外部类型：`One`
- 外部类型：`Referring`
- 外部类型：`Store`
- 外部类型：`dentry`
- 外部类型：`inode`
- 外部类型：`ovl_entry`
- 外部类型：`ovl_fs`
- 外部类型：`ovl_layer`
- 外部类型：`ovl_path`
- 外部类型：`qstr`
- 外部类型：`super_block`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ee87c1279b` → 本草稿移入 `cases/defect/auto-linux-ee87c1279b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
