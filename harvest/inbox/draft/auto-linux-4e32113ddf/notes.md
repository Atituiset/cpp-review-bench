# auto-linux-4e32113ddf

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
| 外部依赖数（dep_count） | 44 |
| 编译错误数（gcc syntax-only） | 57（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #89a312991dc6e638a36adc43ccb91dbc25504c04 (https://github.com/torvalds/linux/commit/89a312991dc6e638a36adc43ccb91dbc25504c04)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4198；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 89a312991dc6e638a36adc43ccb91dbc25504c04 Merge tag 'cifs-fixes-7.3-rc2' of https://git.manguebit.org/linux :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4189,14 +4189,25 @@ cifs_setup_session(const unsigned int xid, struct cifs_ses *ses,
 	return rc;
 }
 
-static int
-cifs_set_vol_auth(struct smb3_fs_context *ctx, struct cifs_ses *ses)
+static int set_fs_context_auth(struct smb3_fs_context *ctx,
+			       struct cifs_ses *ses)
 {
 	ctx->sectype = ses->sectype;
 
-	/* krb5 is special, since we don't need username or pw */
-	if (ctx->sectype == Kerberos)
+	/*
+	 * krb5 is special as we might need to pass username (passwordless) down
+	 * to cifs.upcall(8) for keytab.
+	 */
+	if (ctx->sectype == Kerberos) {
+		if (ses->user_name && ses->user_name[0]) {
+			ctx->username = kstrndup(ses->user_name,
+						 CIFS_MAX_USERNAME_LEN,
+						 GFP_KERNEL);
+			if (!ctx->username)
+				return -ENOMEM;
+		}
 		return 0;
+	}
 
 	return cifs_set_cifscreds(ctx, ses);
 }
@@ -4236,7 +4247,7 @@ cifs_construct_tcon(struct cifs_sb_info *cifs_sb, kuid_t fsuid)
 	ctx->dfs_root_ses = master_tcon->ses->dfs_root_ses;
 	ctx->unicode = master_tcon->ses->unicode;
 
-	rc = cifs_set_vol_auth(ctx, master_tcon->ses);
+	rc = set_fs_context_auth(ctx, master_tcon->ses);
 	if (rc) {
 		tcon = ERR_PTR(rc);
 		goto out;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cifs_dbg`
- 外部函数：`down_read`
- 外部函数：`key_put`
- 外部函数：`kfree`
- 外部函数：`kfree_sensitive`
- 外部函数：`kmalloc`
- 外部函数：`kstrdup`
- 外部函数：`kstrndup`
- 外部函数：`password2`
- 外部函数：`payload`
- 外部函数：`request_key`
- 外部函数：`search`
- 外部函数：`snprintf`
- 外部函数：`ss_family`
- 外部函数：`strlen`
- 外部函数：`strnchr`
- 外部函数：`strscpy`
- 外部函数：`up_read`
- 外部函数：`user_key_payload_locked`
- 大写宏：`AF_INET`
- 大写宏：`AF_INET6`
- 大写宏：`CIFS_MAX_DOMAINNAME_LEN`
- 大写宏：`CIFS_MAX_PASSWORD_LEN`
- 大写宏：`CIFS_MAX_USERNAME_LEN`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`ERR_PTR`
- 大写宏：`FYI`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ERR`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`NUL`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 外部类型：`Bad`
- 外部类型：`If`
- 外部类型：`Kerberos`
- 外部类型：`Key`
- 外部类型：`Rotation`
- 外部类型：`TCP_Server_Info`
- 外部类型：`Unable`
- 外部类型：`cifs_ses`
- 外部类型：`key`
- 外部类型：`size_t`
- 外部类型：`smb3_fs_context`
- 外部类型：`sockaddr_in`
- 外部类型：`sockaddr_in6`
- 外部类型：`ssize_t`
- 外部类型：`user_key_payload`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4e32113ddf` → 本草稿移入 `cases/defect/auto-linux-4e32113ddf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
