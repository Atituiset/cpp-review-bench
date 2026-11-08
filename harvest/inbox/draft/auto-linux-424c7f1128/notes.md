# auto-linux-424c7f1128

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7f75cd8ccd9588e969158c5dc92a8cc217663f5b](https://github.com/torvalds/linux/commit/7f75cd8ccd9588e969158c5dc92a8cc217663f5b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 30 |
| 编译错误数（gcc syntax-only） | 31（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7f75cd8ccd9588e969158c5dc92a8cc217663f5b (https://github.com/torvalds/linux/commit/7f75cd8ccd9588e969158c5dc92a8cc217663f5b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1520；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7f75cd8ccd9588e969158c5dc92a8cc217663f5b Merge tag 'ksmbd-for-7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/smb :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1477,6 +1477,7 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 	int i, found = 0;
 	unsigned int access_bits = 0, denied = 0;
 	struct smb_ace *others_ace = NULL;
+	bool or_ace = false;
 	struct posix_acl_entry *pa_entry;
 	unsigned int sid_type = SIDOWNER;
 	unsigned short ace_size;
@@ -1509,6 +1510,8 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 	is_owner = uid == from_kuid(&init_user_ns, vfsuid_into_kuid(vfsuid));
 
 	if (!pdacl->num_aces) {
+		unsigned int allowed = 0;
+
 		/*
 		 * An empty (present, zero-ACE) DACL grants no access to
 		 * anyone except the object owner's implicit READ_CONTROL
@@ -1517,9 +1520,14 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 		 * not let trailing bytes after a zero-ACE DACL become an
 		 * implicit grant.
 		 */
-		if (!is_owner ||
-		    (*pdaccess & ~(FILE_READ_CONTROL_LE | FILE_WRITE_DAC_LE)))
+		if (is_owner)
+			allowed = READ_CONTROL | WRITE_DAC;
+		if (*pdaccess & FILE_MAXIMAL_ACCESS_LE)
+			requested |= allowed;
+		if (!allowed || requested & ~allowed)
 			rc = -EACCES;
+		else
+			*pdaccess = cpu_to_le32(requested);
 		goto err_out;
 	}
 
@@ -1604,6 +1612,9 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 			found = 1;
 			break;
 		}
+		if (!compare_sids(&sid_owner_rights, &ace->sid) &&
+		    !(ace->flags & INHERIT_ONLY_ACE))
+			or_ace = true;
 		if (!compare_sids(&sid_everyone, &ace->sid) ||
 		    !compare_sids(&sid_authusers, &ace->sid))
 			others_ace = ace;
@@ -1625,7 +1636,10 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 				else
 					continue;
 
-				if (id == uid) {
+				/* A group entry grants only to the group's members */
+				if (id == uid &&
+				    (pa_entry->e_tag == ACL_USER ||
+				     in_group_p(make_kgid(&init_user_ns, id)))) {
 					mode_to_access_flags(pa_entry->e_perm,
 							     0777,
 							     &access_bits);
@@ -1645,8 +1659,23 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 		if (others_ace) {
 			ace = others_ace;
 		} else {
-			ksmbd_debug(SMB, "Can't find corresponding sid\n");
-			rc = -EACCES;
+			unsigned int allowed = 0;
+
+			/*
+			 * No ACE names the owner, who may still read and
+			 * change the DACL unless OWNER RIGHTS applies
+			 * (MS-DTYP 2.5.3.2).
+			 */
+			if (is_owner && !or_ace)
+				allowed = READ_CONTROL | WRITE_DAC;
+			if (*pdaccess & FILE_MAXIMAL_ACCESS_LE)
+				requested |= allowed;
+			if (!allowed || requested & ~allowed) {
+				ksmbd_debug(SMB, "Can't find corresponding sid\n");
+				rc = -EACCES;
+				goto err_out;
+			}
+			*pdaccess = cpu_to_le32(requested);
 			goto err_out;
 		}
 	}
@@ -1672,8 +1701,8 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 	}
 
 	if (granted & ~access_bits) {
-		ksmbd_debug(SMB, "Access denied with winACL, granted : %x, access_req : %x\n",
-			    granted, le32_to_cpu(ace->access_req));
+		ksmbd_debug(SMB, "Access denied with winACL, requested : %x, allowed : %x\n",
+			    granted, access_bits);
 		rc = -EACCES;
 		goto err_out;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`empty`
- 外部函数：`from_kuid`
- 外部函数：`ksmbd_debug`
- 外部函数：`le32_to_cpu`
- 外部函数：`min`
- 外部函数：`vfsuid_into_kuid`
- 大写宏：`ACE`
- 大写宏：`DACL`
- 大写宏：`EACCES`
- 大写宏：`FILE_READ_CONTROL_LE`
- 大写宏：`FILE_WRITE_DAC_LE`
- 大写宏：`FILE_WRITE_RIGHTS`
- 大写宏：`NULL`
- 大写宏：`NUM_AUTHS`
- 大写宏：`READ_CONTROL`
- 大写宏：`RWX`
- 大写宏：`SET_FILE_EXEC_RIGHTS`
- 大写宏：`SET_FILE_READ_RIGHTS`
- 大写宏：`SIDOWNER`
- 大写宏：`SMB`
- 大写宏：`S_IRWXG`
- 大写宏：`S_IRWXO`
- 大写宏：`S_IRWXU`
- 大写宏：`UGO`
- 外部类型：`Access`
- 外部类型：`An`
- 外部类型：`Can`
- 外部类型：`posix_acl_entry`
- 外部类型：`smb_ace`
- 外部类型：`smb_sid`
- 外部类型：`umode_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-424c7f1128` → 本草稿移入 `cases/defect/auto-linux-424c7f1128/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
