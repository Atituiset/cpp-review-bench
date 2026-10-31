# auto-linux-1fb78b6890

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#60aba79a50127e92d8c4a663d35b855e52a9da01](https://github.com/torvalds/linux/commit/60aba79a50127e92d8c4a663d35b855e52a9da01) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #60aba79a50127e92d8c4a663d35b855e52a9da01 (https://github.com/torvalds/linux/commit/60aba79a50127e92d8c4a663d35b855e52a9da01)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 428；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 60aba79a50127e92d8c4a663d35b855e52a9da01 Merge tag 'apparmor-pr-2026-10-29' of git://git.kernel.org/pub/scm/linux/kernel/git/jj/linux-apparmor :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -196,6 +196,9 @@ static int bind_map_addr(const struct sock *sk, struct sockaddr *addr,
 		 * AF_UNSPEC if the address is INADDR_ANY
 		 */
 		addr4 = (struct sockaddr_in *)addr;
+		/* Length check from inet_bind_sk() */
+		if (addrlen < sizeof(struct sockaddr_in))
+			return -EINVAL;
 		if (addr4->sin_addr.s_addr != htonl(INADDR_ANY))
 			return -EAFNOSUPPORT;
 		family = AF_INET;
@@ -425,7 +428,7 @@ int aa_inet_connect_perm(struct socket *sock, struct sockaddr *addr,
 	if (addrlen < offsetofend(struct sockaddr, sa_family))
 		return -EINVAL;
 	if (addr->sa_family == AF_UNSPEC)
-		return 0;
+		return aa_sk_perm(OP_CONNECT, AA_MAY_CONNECT, sock->sk);
 
 	/* do we need early bailout for !family ... */
 	return sk_has_perm2(sock->sk, OP_CONNECT, AA_MAY_CONNECT, profile, ad,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`aa_sock`
- 外部函数：`begin_current_label_crit_section`
- 外部函数：`current_cred`
- 外部函数：`end_current_label_crit_section`
- 外部函数：`fn_for_each`
- 外部函数：`htonl`
- 外部函数：`label_mediates`
- 外部函数：`label_sk_has_perm2`
- 外部函数：`offsetofend`
- 外部函数：`rcu_access_pointer`
- 外部函数：`sk_has_perm2`
- 大写宏：`AAD`
- 大写宏：`AA_CLASS_NET`
- 大写宏：`AA_MAY_CONNECT`
- 大写宏：`AF_INET`
- 大写宏：`AF_UNSPEC`
- 大写宏：`CALLBACKFN`
- 大写宏：`CRED`
- 大写宏：`DEFINE_AUDIT_SK`
- 大写宏：`EAFNOSUPPORT`
- 大写宏：`EINVAL`
- 大写宏：`INADDR_ANY`
- 大写宏：`LABEL`
- 大写宏：`OP_CONNECT`
- 大写宏：`PROFILE`
- 大写宏：`REQUEST`
- 大写宏：`SOCKSK`
- 大写宏：`XXXX`
- 大写宏：`XXXXY`
- 大写宏：`YYYY`
- 大写宏：`YYYYX`
- 外部类型：`aa_label`
- 外部类型：`aa_profile`
- 外部类型：`aa_sk_ctx`
- 外部类型：`kernel_t`
- 外部类型：`sockaddr`
- 外部类型：`sockaddr_in`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1fb78b6890` → 本草稿移入 `cases/defect/auto-linux-1fb78b6890/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
