# auto-redis-e6b4b7d854

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16207](https://github.com/redis/redis/pull/16207) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 27 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16207 (https://github.com/redis/redis/pull/16207)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 529；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16207 Fix replica-side bugs when compression is enabled :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -496,11 +496,12 @@ int decompressInto(compressionState *state, char *buf, size_t buflen) {
 
 /* Read data from input_buf and decompress it immediately. The result is written
  * into output_buf.
- * Return number of bytes decompressed. *consumed stores number of bytes consumed
- * from input_buf.
- * Note, that we may have enough compressed data inside input buf so that decompressing
- * it will exceed output_len. The function must be ran in a loop until input_buf
- * is fully consumed - so make sure to have free space in output_buf on each call. */
+ * Return the number of bytes decompressed, or -1 on decompression failure.
+ * *consumed stores the number of bytes consumed from input_buf.
+ * Consuming input_buf may only copy bytes into the decompressor's internal
+ * buffer. Call again with available output space while input remains or
+ * clientHasPendingCompressedData() is true, using input_len 0 to drain pending
+ * data after input_buf is fully consumed. */
 int clientReadBufAndDecompress(client *c, char *input_buf, size_t input_len,
                              char *output_buf, size_t output_len, size_t *consumed)
 {
@@ -526,12 +527,13 @@ int clientReadBufAndDecompress(client *c, char *input_buf, size_t input_len,
 
         int decompressed = decompressInto(state, output_buf + tot_decompressed,
                                           output_len - tot_decompressed);
-        if (decompressed <= 0) {
-            /* Compression library failure, we should close the connection */
-            if (decompressed < 0)
-                c->conn->state = CONN_STATE_CLOSED;
-            break;
+        if (decompressed < 0) {
+            /* The decoder cannot continue after an error, even if this call
+             * produced some output before reaching the invalid data. */
+            c->conn->state = CONN_STATE_CLOSED;
+            return -1;
         }
+        if (decompressed == 0) break;
         tot_decompressed += decompressed;
     }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`decompress`
- 外部函数：`int`
- 外部函数：`list`
- 外部函数：`memcpy`
- 外部函数：`min`
- 外部函数：`serverAssert`
- 外部函数：`uncompressed`
- 外部函数：`void`
- 大写宏：`CONN_STATE_CLOSED`
- 大写宏：`NULL`
- 外部类型：`Buffer`
- 外部类型：`Compression`
- 外部类型：`Copy`
- 外部类型：`Data`
- 外部类型：`Decompress`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`Read`
- 外部类型：`Reset`
- 外部类型：`Return`
- 外部类型：`The`
- 外部类型：`Time`
- 外部类型：`Total`
- 外部类型：`Used`
- 外部类型：`When`
- 外部类型：`ZSTD_CStream`
- 外部类型：`ZSTD_DStream`
- 外部类型：`ZSTD_e_continue`
- 外部类型：`Zstd`
- 外部类型：`mstime_t`
- 外部类型：`plus`
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

1. 完成上面检查清单后评论 `/case accept auto-redis-e6b4b7d854` → 本草稿移入 `cases/defect/auto-redis-e6b4b7d854/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
