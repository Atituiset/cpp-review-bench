# auto-linux-bcffa4fe10

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ac1bf565f140154ee27db56c80f29c9eeb9a77c3](https://github.com/torvalds/linux/commit/ac1bf565f140154ee27db56c80f29c9eeb9a77c3) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ac1bf565f140154ee27db56c80f29c9eeb9a77c3 (https://github.com/torvalds/linux/commit/ac1bf565f140154ee27db56c80f29c9eeb9a77c3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2337；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ac1bf565f140154ee27db56c80f29c9eeb9a77c3 Merge branch 'fixes-from-sasha' :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2334,9 +2334,6 @@ static struct fuse_conn *fuse_get_conn_for_passthrough(struct file *file)
 	if (!smp_load_acquire(&fud->chan->initialized))
 		return ERR_PTR(-ENOTCONN);
 
-	if (!IS_ENABLED(CONFIG_FUSE_PASSTHROUGH))
-		return ERR_PTR(-EOPNOTSUPP);
-
 	return fud->chan->conn;
 }
 
@@ -2349,6 +2346,9 @@ static long fuse_dev_ioctl_backing_open(struct file *file,
 	if (IS_ERR(fc))
 		return PTR_ERR(fc);
 
+	if (!IS_ENABLED(CONFIG_FUSE_PASSTHROUGH))
+		return -EOPNOTSUPP;
+
 	if (copy_from_user(&map, argp, sizeof(map)))
 		return -EFAULT;
 
@@ -2364,6 +2364,9 @@ static long fuse_dev_ioctl_backing_create(struct file *file,
 	if (IS_ERR(fc))
 		return PTR_ERR(fc);
 
+	if (!IS_ENABLED(CONFIG_FUSE_PASSTHROUGH))
+		return -EOPNOTSUPP;
+
 	if (copy_from_user(&map, argp, sizeof(map)))
 		return -EFAULT;
 
@@ -2378,6 +2381,9 @@ static long fuse_dev_ioctl_backing_close(struct file *file, __u32 __user *argp)
 	if (IS_ERR(fc))
 		return PTR_ERR(fc);
 
+	if (!IS_ENABLED(CONFIG_FUSE_PASSTHROUGH))
+		return -EOPNOTSUPP;
+
 	if (get_user(backing_id, argp))
 		return -EFAULT;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`copy_from_user`
- 外部函数：`get_user`
- 外部函数：`smp_load_acquire`
- 大写宏：`CONFIG_FUSE_PASSTHROUGH`
- 大写宏：`EFAULT`
- 大写宏：`ENOTCONN`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`ERR_PTR`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR`
- 大写宏：`PTR_ERR`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-bcffa4fe10` → 本草稿移入 `cases/defect/auto-linux-bcffa4fe10/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
