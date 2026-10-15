# auto-linux-bb0a3215e1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#89a312991dc6e638a36adc43ccb91dbc25504c04](https://github.com/torvalds/linux/commit/89a312991dc6e638a36adc43ccb91dbc25504c04) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 74 |
| 编译错误数（gcc syntax-only） | 85（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #89a312991dc6e638a36adc43ccb91dbc25504c04 (https://github.com/torvalds/linux/commit/89a312991dc6e638a36adc43ccb91dbc25504c04)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1016；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 89a312991dc6e638a36adc43ccb91dbc25504c04 Merge tag 'cifs-fixes-7.3-rc2' of https://git.manguebit.org/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -999,26 +999,50 @@ static int cifs_do_truncate(const unsigned int xid, struct dentry *dentry)
 	struct cifs_tcon *tcon;
 	int rc;
 
-	rc = filemap_write_and_wait(inode->i_mapping);
-	if (is_interrupt_error(rc))
+	rc = inode_lock_killable(inode);
+	if (rc)
 		return -ERESTARTSYS;
+
+	filemap_invalidate_lock(inode->i_mapping);
+
+	rc = filemap_write_and_wait(inode->i_mapping);
+	if (is_interrupt_error(rc)) {
+		rc = -ERESTARTSYS;
+		goto out;
+	}
 	mapping_set_error(inode->i_mapping, rc);
 
 	cfile = find_writable_file(cinode, FIND_FSUID_ONLY);
 	rc = cifs_file_flush(xid, inode, cfile);
 	if (!rc) {
 		if (cfile) {
+			struct netfs_inode *ictx = netfs_inode(inode);
+
 			tcon = tlink_tcon(cfile->tlink);
 			server = tcon->ses->server;
+			netfs_wb_begin(ictx, false);
 			rc = server->ops->set_file_size(xid, tcon,
 							cfile, 0, false);
-		}
-		if (!rc) {
-			netfs_resize_file(&cinode->netfs, 0, true);
-			cifs_setsize(inode, 0);
+			if (!rc) {
+				netfs_resize_file(&cinode->netfs, 0, true);
+				cifs_setsize(inode, 0);
+				cifs_invalidate_cache(inode, 0);
+			}
+			netfs_wb_end(ictx);
+		} else {
+			/*
+			 * No cached handle; evict stale pages so they can't
+			 * be served after the file is later extended; let
+			 * the server's O_TRUNC open response set the i_size
+			 */
+			truncate_inode_pages(inode->i_mapping, 0);
 			cifs_invalidate_cache(inode, 0);
 		}
 	}
+
+out:
+	filemap_invalidate_unlock(inode->i_mapping);
+	inode_unlock(inode);
 	if (cfile)
 		cifsFileInfo_put(cfile);
 	return rc;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_free_xid`
- 外部函数：`atomic_dec`
- 外部函数：`cancel_work_sync`
- 外部函数：`cifs_add_pending_open_locked`
- 外部函数：`cifs_dbg`
- 外部函数：`cifs_del_pending_open`
- 外部函数：`cifs_done_oplock_break`
- 外部函数：`cifs_get_writable_file`
- 外部函数：`cifs_invalidate_cache`
- 外部函数：`cifs_put_tlink`
- 外部函数：`cifs_sb_flags`
- 外部函数：`cifs_set_oplock_level`
- 外部函数：`cifs_setsize`
- 外部函数：`close`
- 外部函数：`close_getattr`
- 外部函数：`d_inode`
- 外部函数：`down_write_trylock`
- 外部函数：`dput`
- 外部函数：`filemap_write_and_wait`
- 外部函数：`flush`
- 外部函数：`get_lease_key`
- 外部函数：`get_xid`
- 外部函数：`is_interrupt_error`
- 外部函数：`is_size_safe_to_change`
- 外部函数：`kfree`
- 外部函数：`list_del`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`mapping_set_error`
- 外部函数：`msleep`
- 外部函数：`netfs_resize_file`
- 外部函数：`queue_work`
- 外部函数：`set_bit`
- 外部函数：`set_file_size`
- 外部函数：`smp_load_acquire`
- 外部函数：`smp_store_release`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`tlink_tcon`
- 外部函数：`up_write`
- 外部函数：`wake_up`
- 大写宏：`CIFS_I`
- 大写宏：`CIFS_INO_INVALID_MAPPING`
- 大写宏：`CIFS_MOUNT_NOSSYNC`
- 大写宏：`CIFS_MOUNT_STRICT_IO`
- 大写宏：`CIFS_SB`
- 大写宏：`EAGAIN`
- 大写宏：`EBADF`
- 大写宏：`EBUSY`
- 大写宏：`ERESTARTSYS`
- 大写宏：`FIND_ANY`
- 大写宏：`FIND_FSUID_ONLY`
- 大写宏：`FMODE_WRITE`
- 大写宏：`FYI`
- 大写宏：`OPEN_FMODE`
- 外部类型：`Couldn`
- 外部类型：`Delete`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Pairs`
- 外部类型：`Server`
- 外部类型：`Stamp`
- 外部类型：`TCP_Server_Info`
- 外部类型：`We`
- 外部类型：`cifsFileInfo`
- 外部类型：`cifsInodeInfo`
- 外部类型：`cifsLockInfo`
- 外部类型：`cifs_fid`
- 外部类型：`cifs_pending_open`
- 外部类型：`cifs_sb_info`
- 外部类型：`cifs_tcon`
- 外部类型：`inode`
- 外部类型：`rw_semaphore`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-bb0a3215e1` → 本草稿移入 `cases/defect/auto-linux-bb0a3215e1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
