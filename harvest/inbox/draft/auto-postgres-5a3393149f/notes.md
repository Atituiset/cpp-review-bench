# auto-postgres-5a3393149f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#0cb41ee8070fb0930d2ead13e19304d11d405636](https://github.com/postgres/postgres/commit/0cb41ee8070fb0930d2ead13e19304d11d405636) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0cb41ee8070fb0930d2ead13e19304d11d405636 (https://github.com/postgres/postgres/commit/0cb41ee8070fb0930d2ead13e19304d11d405636)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 387；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0cb41ee8070fb0930d2ead13e19304d11d405636 pg_receivewal: Fix resource leaks and error reporting during cleanup :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -384,7 +384,7 @@ dir_write(Walfile *f, const void *buf, size_t count)
 static int
 dir_close(Walfile *f, WalCloseMethod method)
 {
-	int			r;
+	int			r = -1;
 	DirectoryMethodFile *df = (DirectoryMethodFile *) f;
 	DirectoryMethodData *dir_data = (DirectoryMethodData *) f->wwmethod;
 	char		tmppath[MAXPGPATH];
@@ -413,15 +413,17 @@ dir_close(Walfile *f, WalCloseMethod method)
 		if (LZ4F_isError(compressed))
 		{
 			f->wwmethod->lasterrstring = LZ4F_getErrorName(compressed);
-			return -1;
+			(void) close(df->fd);
+			goto cleanup;
 		}
 
 		errno = 0;
 		if (write(df->fd, df->lz4buf, compressed) != compressed)
 		{
 			/* If write didn't set errno, assume problem is no disk space */
 			f->wwmethod->lasterrno = errno ? errno : ENOSPC;
-			return -1;
+			(void) close(df->fd);
+			goto cleanup;
 		}
 
 		r = close(df->fd);
@@ -497,6 +499,7 @@ dir_close(Walfile *f, WalCloseMethod method)
 		f->wwmethod->lasterrno = errno;
 
 #ifdef USE_LZ4
+cleanup:
 	pg_free(df->lz4buf);
 	/* supports free on NULL */
 	LZ4F_freeCompressionContext(df->ctx);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`close`
- 外部函数：`pg_free`
- 外部函数：`write`
- 大写宏：`ENOSPC`
- 大写宏：`HAVE_LIBZ`
- 大写宏：`MAXPGPATH`
- 大写宏：`NULL`
- 大写宏：`USE_LZ4`
- 外部类型：`DirectoryMethodData`
- 外部类型：`DirectoryMethodFile`
- 外部类型：`If`
- 外部类型：`LZ4F_compressionContext_t`
- 外部类型：`WalCloseMethod`
- 外部类型：`WalWriteMethod`
- 外部类型：`Walfile`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-5a3393149f` → 本草稿移入 `cases/defect/auto-postgres-5a3393149f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
