# auto-linux-e90cd365ec

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
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #aacb7797504183a90d66b19b1efb555cfe9b3eec (https://github.com/torvalds/linux/commit/aacb7797504183a90d66b19b1efb555cfe9b3eec)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 79；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT aacb7797504183a90d66b19b1efb555cfe9b3eec Merge tag 'ovl-fixes-7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/overlayfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -76,9 +76,15 @@ enum ovl_entry_flag {
 };
 
 enum {
-	OVL_REDIRECT_OFF,	/* "off" mode is never used. In effect	*/
-	OVL_REDIRECT_FOLLOW,	/* ...it translates to either "follow"	*/
-	OVL_REDIRECT_NOFOLLOW,	/* ...or "nofollow".			*/
+	/*
+	 * "off" mode is never used after mount options are verified.
+	 * In effect it translates to either "follow", "nofollow" or
+	 * "followdata" (with data-only layers).
+	 */
+	OVL_REDIRECT_OFF,
+	OVL_REDIRECT_FOLLOW,
+	OVL_REDIRECT_FOLLOWDATA,
+	OVL_REDIRECT_NOFOLLOW,
 	OVL_REDIRECT_ON,
 };
 
@@ -582,6 +588,7 @@ static inline int ovl_set_impure_dir(struct dentry *dentry)
 bool ovl_inuse_trylock(struct dentry *dentry);
 void ovl_inuse_unlock(struct dentry *dentry);
 bool ovl_is_inuse(struct dentry *dentry);
+void ovl_inuse_unlock_all(struct ovl_fs *ofs);
 bool ovl_need_index(struct dentry *dentry);
 int ovl_nlink_start(struct dentry *dentry);
 void ovl_nlink_end(struct dentry *dentry);
@@ -632,11 +639,20 @@ static inline char ovl_get_opaquedir_val(struct ovl_fs *ofs,
 	return ovl_get_dir_xattr_val(ofs, path, OVL_XATTR_OPAQUE);
 }
 
+/* Are redirects followed into lower layers? */
+static inline bool ovl_redirect_follow_lower(struct ovl_fs *ofs)
+{
+	return ofs->config.redirect_mode == OVL_REDIRECT_FOLLOW ||
+	       ofs->config.redirect_mode == OVL_REDIRECT_ON;
+}
+
+/* Are redirects followed into lower or lowerdata layers? */
 static inline bool ovl_redirect_follow(struct ovl_fs *ofs)
 {
 	return ofs->config.redirect_mode != OVL_REDIRECT_NOFOLLOW;
 }
 
+/* Are redirects created in upper dir? */
 static inline bool ovl_redirect_dir(struct ovl_fs *ofs)
 {
 	return ofs->config.redirect_mode == OVL_REDIRECT_ON;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ovl_get_dir_xattr_val`
- 外部函数：`ovl_inuse_trylock`
- 外部函数：`ovl_inuse_unlock`
- 外部函数：`ovl_is_inuse`
- 外部函数：`ovl_need_index`
- 外部函数：`ovl_nlink_end`
- 外部函数：`ovl_nlink_start`
- 大写宏：`OVL_REDIRECT_FOLLOW`
- 大写宏：`OVL_REDIRECT_NOFOLLOW`
- 大写宏：`OVL_REDIRECT_OFF`
- 大写宏：`OVL_REDIRECT_ON`
- 大写宏：`OVL_XATTR_OPAQUE`
- 外部类型：`In`
- 外部类型：`dentry`
- 外部类型：`ovl_fs`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e90cd365ec` → 本草稿移入 `cases/defect/auto-linux-e90cd365ec/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
