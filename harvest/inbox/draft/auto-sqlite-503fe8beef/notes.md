# auto-sqlite-503fe8beef

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#90ba7148c6d943ec5233002927c18e57cc8a5589](https://github.com/sqlite/sqlite/commit/90ba7148c6d943ec5233002927c18e57cc8a5589) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #90ba7148c6d943ec5233002927c18e57cc8a5589 (https://github.com/sqlite/sqlite/commit/90ba7148c6d943ec5233002927c18e57cc8a5589)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 19（原始 PR diff 行 164；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 90ba7148c6d943ec5233002927c18e57cc8a5589 Merge all recent trunk changes into this branch. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -154,41 +154,58 @@ static char * skipNonB64( char *s, int nc ){
   return s;
 }
 
-/* Decode base64 text into a byte buffer. */
+/* Decode base64 text into a byte buffer.
+**
+** Whitespace is ignored wherever it appears, including inside a group
+** of four base64 digits.  A pad character fills one position of its
+** group with zero bits.  Any other non-base64 character inside a group
+** ends the decode.  Each group yields only the whole bytes encoded by
+** the base64 digits actually present in it.
+*/
 static u8* fromBase64( char *pIn, int ncIn, u8 *pOut ){
   if( ncIn>0 && pIn[ncIn-1]=='\n' ) --ncIn;
   while( ncIn>0 && *pIn!=PAD_CHAR ){
     static signed char nboi[] = { 0, 0, 1, 2, 3 };
     char *pUse = skipNonB64(pIn, ncIn);
     unsigned long qv = 0L;
-    int nti, nbo, nac;
+    int nbo, nac = 0, ndg = 0;
     ncIn -= (pUse - pIn);
     pIn = pUse;
-    nti = (ncIn>4)? 4 : ncIn;
-    ncIn -= nti;
-    nbo = nboi[nti];
-    if( nbo==0 ) break;
-    for( nac=0; nac<4; ++nac ){
-      char c = (nac<nti)? *pIn++ : b64Numerals[0];
-      u8 bdp = BX_DV_PROTO(c);
-      switch( bdp ){
-      case ND:
-        /*  Treat dark non-digits as pad, but they terminate decode too. */
-        ncIn = 0;
-        deliberate_fall_through; /* FALLTHRU */
-      case WS:
-        /* Treat whitespace as pad and terminate this group.*/
-        nti = nac;
-        deliberate_fall_through; /* FALLTHRU */
-      case PC:
-        bdp = 0;
-        --nbo;
-        deliberate_fall_through; /* FALLTHRU */
-      default: /* bdp is the digit value. */
+    if( ncIn>=4 ){
+      /* Fast path: a complete group of four base64 digits. */
+      u8 d0 = BX_DV_PROTO(pIn[0]), d1 = BX_DV_PROTO(pIn[1]);
+      u8 d2 = BX_DV_PROTO(pIn[2]), d3 = BX_DV_PROTO(pIn[3]);
+      if( IS_BX_DIGIT(d0|d1|d2|d3) ){
+        qv = (unsigned long)d0<<18 | (unsigned long)d1<<12 | d2<<6 | d3;
+        pOut[0] = (qv>>16) & 0xff;
+        pOut[1] = (qv>>8) & 0xff;
+        pOut[2] = (qv) & 0xff;
+        pOut += 3;
+        pIn += 4;
+        ncIn -= 4;
+        continue;
+      }
+    }
+    /* Gather up to four group positions: digits and pad characters. */
+    while( nac<4 && ncIn>0 ){
+      u8 bdp = BX_DV_PROTO(*pIn);
+      if( IS_BX_DIGIT(bdp) ){
         qv = qv<<6 | bdp;
+        ++nac;
+        ++ndg;
+      }else if( IS_BX_PAD(bdp) ){
+        qv <<= 6;
+        ++nac;
+      }else if( !IS_BX_WS(bdp) ){
+        /* Dark non-digits terminate the decode. */
+        ncIn = 0;
         break;
       }
+      ++pIn;
+      --ncIn;
     }
+    qv <<= 6*(4-nac);
+    nbo = nboi[ndg];
     switch( nbo ){
     case 3:
       pOut[2] = (qv) & 0xff;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__attribute__`
- 大写宏：`FALLTHRU`
- 外部类型：`Decode`
- 外部类型：`Not`
- 外部类型：`Treat`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-503fe8beef` → 本草稿移入 `cases/defect/auto-sqlite-503fe8beef/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
