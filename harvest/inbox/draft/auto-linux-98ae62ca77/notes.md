# auto-linux-98ae62ca77

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#62b13f41be6284250c06ab4d79a7fb13fe93f4d7](https://github.com/torvalds/linux/commit/62b13f41be6284250c06ab4d79a7fb13fe93f4d7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 194 |
| 编译错误数（gcc syntax-only） | 310（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #62b13f41be6284250c06ab4d79a7fb13fe93f4d7 (https://github.com/torvalds/linux/commit/62b13f41be6284250c06ab4d79a7fb13fe93f4d7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 62b13f41be6284250c06ab4d79a7fb13fe93f4d7 Merge tag 'cifs-fixes-7.4-rc4' of https://git.manguebit.org/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2044,6 +2044,7 @@ bool
 cifs_reopen_persistent_handles(struct cifs_tcon *tcon)
 {
 	struct cifsFileInfo *open_file, *tmp;
+	struct cifs_sb_info *cifs_sb;
 	LIST_HEAD(tmp_list);
 	bool retry = false;
 	int rc;
@@ -2060,12 +2061,15 @@ cifs_reopen_persistent_handles(struct cifs_tcon *tcon)
 	list_for_each_entry(open_file, &tcon->openFileList, tlist) {
 		if (!open_file->invalidHandle)
 			continue;
+		cifs_sb = CIFS_SB(open_file->dentry);
 		cifsFileInfo_get(open_file);
+		atomic_inc(&cifs_sb->outstanding_rreq);
 		list_add_tail(&open_file->rlist, &tmp_list);
 	}
 	spin_unlock(&tcon->open_file_lock);
 
 	list_for_each_entry_safe(open_file, tmp, &tmp_list, rlist) {
+		cifs_sb = CIFS_SB(open_file->dentry);
 		/* do not flush */
 		rc = cifs_reopen_file(open_file, false,
 				      CIFS_REOPEN_BOUND | CIFS_REOPEN_NOWAIT);
@@ -2075,6 +2079,8 @@ cifs_reopen_persistent_handles(struct cifs_tcon *tcon)
 		}
 		list_del_init(&open_file->rlist);
 		cifsFileInfo_put(open_file);
+		if (atomic_dec_and_test(&cifs_sb->outstanding_rreq))
+			wake_up_var(&cifs_sb->outstanding_rreq);
 	}
 	return retry;
 }
@@ -4023,6 +4029,8 @@ void cifs_oplock_break(struct work_struct *work)
 out:
 	cifs_done_oplock_break(cinode);
 	iput(inode);
+	if (atomic_dec_and_test(&cifs_sb->outstanding_rreq))
+		wake_up_var(&cifs_sb->outstanding_rreq);
 }
 
 #ifdef CONFIG_SWAP
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cifsFileInfo_get_locked`
- 外部函数：`cifs_dbg`
- 外部函数：`cifs_del_lock_waiters`
- 外部函数：`cifs_fattr_to_inode`
- 外部函数：`cifs_fill_uniqueid`
- 外部函数：`cifs_iget`
- 外部函数：`cifs_put_tlink`
- 外部函数：`cifs_remap`
- 外部函数：`cifs_revalidate_mapping`
- 外部函数：`cifs_sb_tlink`
- 外部函数：`cifs_unix_basic_to_fattr`
- 外部函数：`cpu_to_le32`
- 外部函数：`current_umask`
- 外部函数：`d_inode`
- 外部函数：`down_read`
- 外部函数：`down_write_trylock`
- 外部函数：`dput`
- 外部函数：`get_lease_key`
- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`msleep`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`tlink_tcon`
- 外部函数：`up_read`
- 外部函数：`up_write`
- 大写宏：`BIT`
- 大写宏：`CIFS_I`
- 大写宏：`CIFS_SB`
- 大写宏：`DELETE`
- 大写宏：`ENOMEM`
- 大写宏：`FILE_APPEND_DATA`
- 大写宏：`FILE_READ_ATTRIBUTES`
- 大写宏：`FILE_READ_DATA`
- 大写宏：`FILE_UNIX_BASIC_INFO`
- 大写宏：`FILE_WRITE_ATTRIBUTES`
- 大写宏：`FILE_WRITE_DATA`
- 大写宏：`FILE_WRITE_EA`
- 大写宏：`FYI`
- 大写宏：`GENERIC_ALL`
- 大写宏：`GENERIC_READ`
- 大写宏：`GENERIC_WRITE`
- 大写宏：`IS_ERR`
- 大写宏：`NULL`
- 大写宏：`O_ACCMODE`
- 大写宏：`O_CREAT`
- 大写宏：`O_DIRECT`
- 大写宏：`O_DIRECTORY`
- 大写宏：`O_DSYNC`
- 大写宏：`O_EXCL`
- 大写宏：`O_NOFOLLOW`
- 大写宏：`O_RDONLY`
- 大写宏：`O_RDWR`
- 大写宏：`O_SYNC`
- 大写宏：`O_TMPFILE`
- 大写宏：`O_TRUNC`
- 大写宏：`O_WRONLY`
- 大写宏：`PTR_ERR`
- 大写宏：`READ_CONTROL`
- 大写宏：`SMB_O_CREAT`
- 大写宏：`SMB_O_DIRECT`
- 大写宏：`SMB_O_DIRECTORY`
- 大写宏：`SMB_O_EXCL`
- 大写宏：`SMB_O_NOFOLLOW`
- 大写宏：`SMB_O_RDONLY`
- 大写宏：`SMB_O_RDWR`
- 大写宏：`SMB_O_SYNC`
- 大写宏：`SMB_O_TRUNC`
- 大写宏：`SMB_O_WRONLY`
- 外部类型：`Application`
- 外部类型：`Delete`
- 外部类型：`Ignoring`
- 外部类型：`TCP_Server_Info`
- 外部类型：`Type`
- 外部类型：`We`
- 外部类型：`cifsFileInfo`
- 外部类型：`cifsInodeInfo`
- 外部类型：`cifsLockInfo`
- 外部类型：`cifs_fattr`
- 外部类型：`cifs_fid`
- 外部类型：`cifs_fid_locks`
- 外部类型：`cifs_pending_open`
- 外部类型：`cifs_sb_info`
- 外部类型：`cifs_tcon`
- 外部类型：`inode`
- 外部类型：`rw_semaphore`
- 外部类型：`super_block`
- 外部类型：`tcon_link`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-98ae62ca77` → 本草稿移入 `cases/defect/auto-linux-98ae62ca77/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
