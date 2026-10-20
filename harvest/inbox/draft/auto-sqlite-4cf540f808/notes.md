# auto-sqlite-4cf540f808

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c01201027e319addcc7a5fc811ecc5cbea6074b2](https://github.com/sqlite/sqlite/commit/c01201027e319addcc7a5fc811ecc5cbea6074b2) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c01201027e319addcc7a5fc811ecc5cbea6074b2 (https://github.com/sqlite/sqlite/commit/c01201027e319addcc7a5fc811ecc5cbea6074b2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 245；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c01201027e319addcc7a5fc811ecc5cbea6074b2 Wasm build: move wasm-strip to after wasm-opt, at recommendation from an Emscripten ticket. Remove several more uses of  :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -242,7 +242,7 @@ const BuildDefs oBuildDefs = {
     .zDotWasm    = 0,
     .zCmppD      = 0,
     .zEmcc       = 0,
-    .zEmccExtra  = "$(emcc.MEMORY64) -sWASM_BIGINT=1",
+    .zEmccExtra  = "$(emcc.MEMORY64)",
     .zEnv        = 0,
     .zDeps       = 0,
     .zIfCond     = 0,
@@ -270,7 +270,7 @@ const BuildDefs oBuildDefs = {
     .zDotWasm    = 0,
     .zCmppD      = "-Dtarget:es6-module",
     .zEmcc       = 0,
-    .zEmccExtra  = "$(emcc.MEMORY64) -sWASM_BIGINT=1",
+    .zEmccExtra  = "$(emcc.MEMORY64)",
     .zEnv        = 0,
     .zDeps       = 0,
     .zIfCond     = 0,
@@ -313,7 +313,7 @@ const BuildDefs oBuildDefs = {
     .zEmcc       =
     "$(emcc.speedtest1)"
     " $(emcc.speedtest1.common)"
-    " $(emcc.MEMORY64) -sWASM_BIGINT=1"
+    " $(emcc.MEMORY64)"
     " $(pre-post.speedtest164.flags)"
     " $(cflags.common)"
     " -DSQLITE_SPEEDTEST1_WASM"
@@ -496,11 +496,15 @@ static void mk_prologue(void){
       ** Performance gains or losses are _not_ taken into account
       ** here, only wasm file size.
       */
+#if 1
+      /* -Oz builds fail to build if we remove this block, for
+         as-yet-unknown reasons. */
       "--enable-bulk-memory-opt " /* required */
       "--all-features "           /* required */
       "--disable-compact-imports " /*
          https://github.com/emscripten-core/emscripten/issues/28016
       */
+#endif
       "--post-emscripten "        /* Saves roughly 12kb */
       "--strip-debug "            /* We already wasm-strip, but in
                                   ** case this environment has no
@@ -880,9 +884,8 @@ static void mk_lib_mode(const char *zBuildName, const BuildDef * pB){
          /* althttpd will automatically try to execute wasm files
             if they have the +x bit set. Why that bit is set
             at all is a mystery. */);
-      pf("\t@$(call b.call.wasm-strip,%s)\n", zBuildName);
-
       pf("\t@$(call b.do.wasm-opt,%s)\n", zBuildName);
+      pf("\t@$(call b.call.wasm-strip,%s)\n", zBuildName);
       pf("\t@$(call b.strip-js-emcc-bindings,$(logtag.%s))\n", zBuildName);
 
       pf("\t@if [ x1 = 'x$(base64.wasm)' ]; then \\\n"
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pf`
- 大写宏：`DSQLITE_SPEEDTEST1_WASM`
- 大写宏：`MEMORY64`
- 外部类型：`Dtarget`
- 外部类型：`Performance`
- 外部类型：`Saves`
- 外部类型：`We`
- 外部类型：`Why`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-4cf540f808` → 本草稿移入 `cases/defect/auto-sqlite-4cf540f808/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
