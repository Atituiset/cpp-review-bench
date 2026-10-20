# auto-linux-8caefc4641

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2a49d49159b76f6dd587131b3765d3458cc3aa01](https://github.com/torvalds/linux/commit/2a49d49159b76f6dd587131b3765d3458cc3aa01) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2a49d49159b76f6dd587131b3765d3458cc3aa01 (https://github.com/torvalds/linux/commit/2a49d49159b76f6dd587131b3765d3458cc3aa01)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 139；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2a49d49159b76f6dd587131b3765d3458cc3aa01 Merge tag 'vfs-7.4-rc1.netfs' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -34,6 +34,57 @@ struct cachefiles_vol_xattr {
 	__u8	data[];		/* netfs volume coherency data */
 } __packed;
 
+/*
+ * Preset the state xattr on a cache file to allocate space for it.
+ */
+int cachefiles_preset_object_xattr(struct cachefiles_object *object, struct file *file)
+{
+	struct cachefiles_xattr *buf;
+	struct dentry *dentry = file->f_path.dentry;
+	unsigned int len = object->cookie->aux_len;
+	int ret;
+
+	buf = kzalloc(sizeof(struct cachefiles_xattr) + min(len, sizeof(__be64)), GFP_KERNEL);
+	if (!buf)
+		return -ENOMEM;
+
+	buf->type		= CACHEFILES_COOKIE_TYPE_DATA;
+	buf->content		= CACHEFILES_CONTENT_DIRTY;
+
+	ret = cachefiles_inject_write_error();
+	if (ret == 0) {
+		ret = mnt_want_write_file(file);
+		if (ret == 0) {
+			ret = vfs_setxattr(&nop_mnt_idmap, dentry,
+					   cachefiles_xattr_cache, buf,
+					   sizeof(struct cachefiles_xattr) + len, 0);
+			mnt_drop_write_file(file);
+		}
+	}
+	if (ret < 0) {
+		trace_cachefiles_vfs_error(object, file_inode(file), ret,
+					   cachefiles_trace_setxattr_error);
+		trace_cachefiles_coherency(object, file_inode(file)->i_ino,
+					   object->object_size,
+					   buf->data, buf->content,
+					   cachefiles_coherency_set_fail);
+		switch (ret) {
+		case -ENOMEM:
+		case -ENOSPC:
+			break;
+		default:
+			cachefiles_io_error_obj(
+				object,
+				"Failed to set xattr with error %d", ret);
+			break;
+		}
+	}
+
+	kfree(buf);
+	_leave(" = %d", ret);
+	return ret;
+}
+
 /*
  * set the state xattr on a cache file
  */
@@ -43,6 +94,7 @@ int cachefiles_set_object_xattr(struct cachefiles_object *object)
 	struct dentry *dentry;
 	struct file *file = object->file;
 	unsigned int len = object->cookie->aux_len;
+	uoff_t object_size = object->cookie->object_size;
 	int ret;
 
 	if (!file)
@@ -55,7 +107,7 @@ int cachefiles_set_object_xattr(struct cachefiles_object *object)
 	if (!buf)
 		return -ENOMEM;
 
-	buf->object_size	= cpu_to_be64(object->cookie->object_size);
+	buf->object_size	= cpu_to_be64(object_size);
 	buf->zero_point		= 0;
 	buf->type		= CACHEFILES_COOKIE_TYPE_DATA;
 	buf->content		= object->content_info;
@@ -79,15 +131,21 @@ int cachefiles_set_object_xattr(struct cachefiles_object *object)
 		trace_cachefiles_vfs_error(object, file_inode(file), ret,
 					   cachefiles_trace_setxattr_error);
 		trace_cachefiles_coherency(object, file_inode(file)->i_ino,
-					   buf->data, buf->content,
+					   object_size, buf->data, buf->content,
 					   cachefiles_coherency_set_fail);
-		if (ret != -ENOMEM)
+		switch (ret) {
+		case -ENOMEM:
+			break;
+		case -ENOSPC:
+		default:
 			cachefiles_io_error_obj(
 				object,
 				"Failed to set xattr with error %d", ret);
+			break;
+		}
 	} else {
 		trace_cachefiles_coherency(object, file_inode(file)->i_ino,
-					   buf->data, buf->content,
+					   object_size, buf->data, buf->content,
 					   cachefiles_coherency_set_ok);
 	}
 
@@ -103,10 +161,12 @@ int cachefiles_check_auxdata(struct cachefiles_object *object, struct file *file
 {
 	struct cachefiles_xattr *buf;
 	struct dentry *dentry = file->f_path.dentry;
+	struct inode *inode = file_inode(file);
 	unsigned int len = object->cookie->aux_len, tlen;
 	const void *p = fscache_get_aux(object->cookie);
 	enum cachefiles_coherency_trace why;
 	ssize_t xlen;
+	uoff_t obj_size;
 	int ret = -ESTALE;
 
 	tlen = sizeof(struct cachefiles_xattr) + len;
@@ -121,34 +181,39 @@ int cachefiles_check_auxdata(struct cachefiles_object *object, struct file *file
 	if (xlen != tlen) {
 		if (xlen < 0) {
 			ret = xlen;
-			trace_cachefiles_vfs_error(object, file_inode(file), xlen,
+			trace_cachefiles_vfs_error(object, inode, xlen,
 						   cachefiles_trace_getxattr_error);
 		}
 		if (xlen == -EIO)
 			cachefiles_io_error_obj(
 				object,
 				"Failed to read aux with error %zd", xlen);
+		obj_size = 0;
 		why = cachefiles_coherency_check_xattr;
 		goto out;
 	}
 
+	obj_size = be64_to_cpu(buf->object_size);
 	if (buf->type != CACHEFILES_COOKIE_TYPE_DATA) {
 		why = cachefiles_cohe
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`be64_to_cpu`
- 外部函数：`cachefiles_inject_remove_error`
- 外部函数：`cachefiles_io_error_obj`
- 外部函数：`cpu_to_be64`
- 外部函数：`file_inode`
- 外部函数：`fscache_get_aux`
- 外部函数：`kfree`
- 外部函数：`memcmp`
- 外部函数：`mnt_want_write`
- 外部函数：`pr_warn`
- 外部函数：`trace_cachefiles_coherency`
- 外部函数：`trace_cachefiles_vfs_error`
- 大写宏：`CACHEFILES_CONTENT_DIRTY`
- 大写宏：`EIO`
- 大写宏：`ENOMEM`
- 大写宏：`ESTALE`
- 大写宏：`TODO`
- 外部类型：`Begin`
- 外部类型：`Dirty`
- 外部类型：`Failed`
- 外部类型：`cachefiles_coherency_trace`
- 外部类型：`cachefiles_xattr`
- 外部类型：`dentry`
- 外部类型：`file`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-8caefc4641` → 本草稿移入 `cases/defect/auto-linux-8caefc4641/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
