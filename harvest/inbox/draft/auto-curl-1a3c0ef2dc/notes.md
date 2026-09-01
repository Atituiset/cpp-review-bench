# auto-curl-1a3c0ef2dc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#15289](https://github.com/curl/curl/pull/15289) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15289 (https://github.com/curl/curl/pull/15289)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 1（原始 PR diff 行 35；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15289 lib: fix function pointers to please UndefinedBehaviorSanitizer :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -32,30 +32,28 @@
 
 #define HMAC_MD5_LENGTH 16
 
-typedef CURLcode (* HMAC_hinit_func)(void *context);
-typedef void    (* HMAC_hupdate_func)(void *context,
-                                      const unsigned char *data,
-                                      unsigned int len);
-typedef void    (* HMAC_hfinal_func)(unsigned char *result, void *context);
-
+typedef CURLcode (*HMAC_hinit)(void *context);
+typedef void    (*HMAC_hupdate)(void *context,
+                                const unsigned char *data,
+                                unsigned int len);
+typedef void    (*HMAC_hfinal)(unsigned char *result, void *context);
 
 /* Per-hash function HMAC parameters. */
 struct HMAC_params {
-  HMAC_hinit_func
-  hmac_hinit;     /* Initialize context procedure. */
-  HMAC_hupdate_func     hmac_hupdate;   /* Update context with data. */
-  HMAC_hfinal_func      hmac_hfinal;    /* Get final result procedure. */
-  unsigned int          hmac_ctxtsize;  /* Context structure size. */
-  unsigned int          hmac_maxkeylen; /* Maximum key length (bytes). */
-  unsigned int          hmac_resultlen; /* Result length (bytes). */
+  HMAC_hinit       hinit;     /* Initialize context procedure. */
+  HMAC_hupdate     hupdate;   /* Update context with data. */
+  HMAC_hfinal      hfinal;    /* Get final result procedure. */
+  unsigned int     ctxtsize;  /* Context structure size. */
+  unsigned int     maxkeylen; /* Maximum key length (bytes). */
+  unsigned int     resultlen; /* Result length (bytes). */
 };
 
 
 /* HMAC computation context. */
 struct HMAC_context {
-  const struct HMAC_params *hmac_hash; /* Hash function definition. */
-  void *hmac_hashctxt1;         /* Hash function context 1. */
-  void *hmac_hashctxt2;         /* Hash function context 2. */
+  const struct HMAC_params *hash; /* Hash function definition. */
+  void *hashctxt1;         /* Hash function context 1. */
+  void *hashctxt2;         /* Hash function context 2. */
 };
 
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`length`
- 外部函数：`void`
- 大写宏：`HMAC`
- 外部类型：`Context`
- 外部类型：`Get`
- 外部类型：`HMAC_context`
- 外部类型：`HMAC_hfinal_func`
- 外部类型：`HMAC_hinit_func`
- 外部类型：`HMAC_hupdate_func`
- 外部类型：`HMAC_params`
- 外部类型：`Hash`
- 外部类型：`Initialize`
- 外部类型：`Maximum`
- 外部类型：`Per`
- 外部类型：`Result`
- 外部类型：`Update`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-1a3c0ef2dc` → 本草稿移入 `cases/defect/auto-curl-1a3c0ef2dc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
