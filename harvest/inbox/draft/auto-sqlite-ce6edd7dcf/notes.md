# auto-sqlite-ce6edd7dcf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#670fc9f9d52bcf888b7cf515299d511f46609222](https://github.com/sqlite/sqlite/commit/670fc9f9d52bcf888b7cf515299d511f46609222) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #670fc9f9d52bcf888b7cf515299d511f46609222 (https://github.com/sqlite/sqlite/commit/670fc9f9d52bcf888b7cf515299d511f46609222)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 361；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 670fc9f9d52bcf888b7cf515299d511f46609222 Doc-only updates in the JS/WASM pieces. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -297,13 +297,19 @@ SQLITE_WASM_EXPORT2(WasmDisposalNotifier *,
 ** Which sqlite3.c we're using needs to be configurable to enable
 ** building against a custom copy, e.g. the SEE variant. We #include
 ** the .c file, rather than the header, so that the WASM extensions
-** have access to private API internals (namely for kvvfs and
-** SQLTester pieces).
+** have access to private API internals (namely for kvvfs, SQLTester
+** pieces, and SQLITE_DISPOSAL_NOTIFIER). Though kvvfs and the testing
+** pieces are optional, SQLITE_DISPOSAL_NOTIFIER is required for
+** JS-side memory-lifetime guarantees made as of 3.55.0 (and module
+** won't bootstrap if that's not active).
 **
 ** The caveat here is that custom variants need to account for
 ** exporting any necessary symbols (e.g. sqlite3_activate_see()).  We
 ** cannot export them from here using SQLITE_WASM_EXPORT because that
 ** attribute (apparently) has to be part of the function definition.
+** The canonical build process specifically supports SEE, exporting
+** its additional functions when building against an SEE copy of
+** sqlite3.c.
 */
 #ifndef SQLITE_C
 # define SQLITE_C sqlite3.c /* yes, .c instead of .h. */
@@ -358,17 +364,17 @@ SQLITE_WASM_EXPORT void * sqlite3__wasm_pstack_ptr(void){
 SQLITE_WASM_EXPORT void sqlite3__wasm_pstack_restore(unsigned char * p){
   assert(p>=PStack.pBegin && p<=PStack.pEnd && p>=PStack.pPos);
   assert(0==((unsigned long long)p & 0x7) /* 8-byte aligned */);
-  if(p>=PStack.pBegin && p<=PStack.pEnd /*&& p>=PStack.pPos*/){
+  if(p>=PStack.pBegin && p<=PStack.pEnd){
     PStack.pPos = p;
   }
 }
 /*
 ** Allocate and zero out n bytes from the pstack. Returns a pointer to
 ** the memory on success, 0 on error (including a negative n value). n
-** is always adjusted to be a multiple of 8 and returned memory is
-** always zeroed out before returning (because this keeps the client
-** JS code from having to do so, and most uses of the pstack call for
-** doing so).
+** is always adjusted to be a multiple of 8 and returned memory is (A)
+** similarly aligned and (B) always zeroed out before returning
+** (because this keeps the client JS code from having to do so, and
+** most uses of the pstack call for doing so).
 */
 SQLITE_WASM_EXPORT2(void *,sqlite3__wasm_pstack_alloc,(int n)){
   if( n<=0 || n>(1024 * 1024/*https://sqlite.org/bugs/forumpost/4dbd096fe3*/) ){
@@ -442,11 +448,6 @@ SQLITE_WASM_EXPORT2(void,sqlite3__wasm_test_struct,(WasmTestStruct * s)){
 ** If this function returns NULL then it means that the internal
 ** buffer is not large enough for the generated JSON and needs to be
 ** increased. In debug builds that will trigger an assert().
-**
-** 2025-09-19: for reasons entirely not understood, building with emcc
-** -sMEMORY64=2 causes this function to fail (return 0). -sMEMORY64=1
-** fails to compile with "tables may not be 64-bit" but does not tell
-** us where it's happening.
 */
 SQLITE_WASM_EXPORT2(const char *,sqlite3__wasm_enum_json,(void)){
   static char aBuffer[1024 * 24] =
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`attribute`
- 外部函数：`fail`
- 外部函数：`internals`
- 外部函数：`returning`
- 外部函数：`sqlite3__wasm_pstack_restore`
- 外部函数：`sqlite3_activate_see`
- 外部函数：`symbols`
- 外部函数：`visibility`
- 大写宏：`API`
- 大写宏：`JSON`
- 大写宏：`NULL`
- 大写宏：`SEE`
- 大写宏：`SQLITE_WASM_EXPORT2`
- 大写宏：`WASM`
- 外部类型：`Allocate`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`PStack`
- 外部类型：`Returns`
- 外部类型：`SQLTester`
- 外部类型：`The`
- 外部类型：`We`
- 外部类型：`Which`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-ce6edd7dcf` → 本草稿移入 `cases/defect/auto-sqlite-ce6edd7dcf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
