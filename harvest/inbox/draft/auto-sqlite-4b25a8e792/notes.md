# auto-sqlite-4b25a8e792

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#59932cc0c81387b108add8f2568a2b9b6e82313d](https://github.com/sqlite/sqlite/commit/59932cc0c81387b108add8f2568a2b9b6e82313d) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 46 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #59932cc0c81387b108add8f2568a2b9b6e82313d (https://github.com/sqlite/sqlite/commit/59932cc0c81387b108add8f2568a2b9b6e82313d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 59932cc0c81387b108add8f2568a2b9b6e82313d Fix an infinite loop in QRF with .mode box --wrap 1 and double-width characters. [bugs:14c1b50193|Bug report 2026-11-19T :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -765,6 +765,24 @@ static int qrfDisplayWidth(const char *zIn, sqlite3_int64 nByte, int *pnNL){
   return n;
 }
 
+/*
+** Return true if the zero-terminated string zIn contains any character
+** that is more than one column wide.
+*/
+static int qrfHasWideChar(const char *zIn){
+  const unsigned char *z = (const unsigned char*)zIn;
+  while( z[0] ){
+    if( z[0]>=0xc0 ){
+      int u = 0;
+      z += sqlite3_qrf_decode_utf8(z, &u);
+      if( sqlite3_qrf_wcwidth(u)>1 ) return 1;
+    }else{
+      z++;
+    }
+  }
+  return 0;
+}
+
 /*
 ** Escape the text starting at byte iStart of pStr, if needed, using the
 ** escape encoding of eEsc, which is either QRF_ESC_Ascii or QRF_ESC_Symbol.
@@ -2196,6 +2214,17 @@ static void qrfColumnar(Qrf *p){
       if( p->spec.nWrap>0 && w>p->spec.nWrap ){
         w = p->spec.nWrap;
         data.bMultiRow = 1;
+        if( w==1 ){
+          /* A double-wide character needs a column at least 2 wide, so
+          ** that qrfWrapLine() always makes progress. */
+          int j;
+          for(j=i; j<data.n; j+=nColumn){
+            if( qrfHasWideChar(data.az[j]) ){
+              w = 2;
+              break;
+            }
+          }
+        }
       }
     }else if( (data.bMultiRow==0 || w==1) && data.a[i].mxW>w ){
       data.bMultiRow = 1;
@@ -2323,6 +2352,7 @@ static void qrfColumnar(Qrf *p){
         int iNext = 0;
         int nWS;
         qrfWrapLine(data.a[j].z, data.a[j].w, bWW, &nThis, &nWide, &iNext);
+        assert( iNext>0 || data.a[j].z[0]==0 );
         nWS = data.a[j].w - nWide;
         qrfPrintAligned(p->pOut, &data.a[j], nThis, nWS);
         data.a[j].z += iNext;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`characters`
- 外部函数：`qrfAlnum`
- 外部函数：`sqlite3_qrf_wcwidth`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendchar`
- 大写宏：`OUT`
- 大写宏：`VT100`
- 外部类型：`Any`
- 外部类型：`Append`
- 外部类型：`Bytes`
- 外部类型：`Column`
- 外部类型：`Control`
- 外部类型：`Count`
- 外部类型：`Escape`
- 外部类型：`Fast`
- 外部类型：`Figure`
- 外部类型：`How`
- 外部类型：`If`
- 外部类型：`Input`
- 外部类型：`Offset`
- 外部类型：`Output`
- 外部类型：`Perhaps`
- 外部类型：`Picture`
- 外部类型：`QRF_ESC_Ascii`
- 外部类型：`QRF_ESC_Symbol`
- 外部类型：`Text`
- 外部类型：`The`
- 外部类型：`True`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-4b25a8e792` → 本草稿移入 `cases/defect/auto-sqlite-4b25a8e792/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
