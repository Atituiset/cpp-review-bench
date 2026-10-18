# auto-sqlite-e3fbf0c9e5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#8e1b59b6dc35197ec43b4dc2d31000fbd9d4c291](https://github.com/sqlite/sqlite/commit/8e1b59b6dc35197ec43b4dc2d31000fbd9d4c291) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 88 |
| 编译错误数（gcc syntax-only） | 78（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8e1b59b6dc35197ec43b4dc2d31000fbd9d4c291 (https://github.com/sqlite/sqlite/commit/8e1b59b6dc35197ec43b4dc2d31000fbd9d4c291)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 492；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8e1b59b6dc35197ec43b4dc2d31000fbd9d4c291 Enhance QRF to deal with zero-length column names. :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -489,9 +489,9 @@ static void qrfEqpStats(Qrf *p){
   }
   if( p->u.pGraph ) p->u.pGraph->nWidth = nWidth;
   qrfStrErr(p, pLine);
-  sqlite3_free(sqlite3_str_finish(pLine));
+  sqlite3_str_free(pLine);
   qrfStrErr(p, pStats);
-  sqlite3_free(sqlite3_str_finish(pStats));
+  sqlite3_str_free(pStats);
 #endif
 }
 
@@ -2008,6 +2008,21 @@ static void qrfRestrictScreenWidth(qrfColData *pData, Qrf *p){
   }
 }
 
+/*
+** Invoke sqlite3_str_finish().  But if it returns NULL, try to allocate
+** an empty string to replace the result.
+**
+** The result might still be an empty string, but only after an OOM and
+** in that case qrfOom() has been called already and so it is safe to
+** return immediately.
+*/
+static char *qrfStrFinish(Qrf *p, sqlite3_str *pStr){
+  char *z = sqlite3_str_finish(pStr);
+  if( z==0 ) z = sqlite3_mprintf("");
+  if( z==0 ) qrfOom(p);
+  return z;
+}
+
 /*
 ** Columnar modes require that the entire query be evaluated first, with
 ** results written into memory, so that we can compute appropriate column
@@ -2071,7 +2086,8 @@ static void qrfColumnar(Qrf *p){
       qrfEncodeText(p, pStr, z ? z : "");
       n = sqlite3_str_length(pStr);
       qrfStrErr(p, pStr);
-      z = data.az[data.n] = sqlite3_str_finish(pStr);
+      z = data.az[data.n] = qrfStrFinish(p, pStr);
+      if( z==0 ) goto columnar_done;
       if( p->spec.nTitleLimit ){
         nNL = 0;
         data.aiWth[data.n] = w = qrfTitleLimit(data.az[data.n],
@@ -2100,7 +2116,8 @@ static void qrfColumnar(Qrf *p){
       qrfRenderValue(p, pStr, i);
       n = sqlite3_str_length(pStr);
       qrfStrErr(p, pStr);
-      z = data.az[data.n] = sqlite3_str_finish(pStr);
+      z = data.az[data.n] = qrfStrFinish(p,pStr);
+      if( z==0 ) goto columnar_done;
       data.abNum[data.n] = eType==SQLITE_INTEGER || eType==SQLITE_FLOAT;
       data.aiWth[data.n] = w = qrfDisplayWidth(z, n, &nNL);
       data.n++;
@@ -2388,6 +2405,7 @@ static void qrfColumnar(Qrf *p){
   }
   qrfWrite(p);
 
+columnar_done:
   qrfColDataFree(&data);
   return;
 }
@@ -2819,7 +2837,7 @@ static void qrfOneSimpleRow(Qrf *p){
         sqlite3_str_reset(pVal);
       }
       qrfStrErr(p, pVal);
-      sqlite3_free(sqlite3_str_finish(pVal));
+      sqlite3_str_free(pVal);
       qrfWrite(p);
       break;
     }
@@ -3102,12 +3120,12 @@ static void qrfFinalize(Qrf *p){
         p->spec.pzOutput[0] = zCombined;
         memcpy(zCombined+sz, sqlite3_str_value(p->pOut), n+1);
       }
-      sqlite3_free(sqlite3_str_finish(p->pOut));
+      sqlite3_str_free(p->pOut);
     }else{
-      p->spec.pzOutput[0] = sqlite3_str_finish(p->pOut);
+      p->spec.pzOutput[0] = qrfStrFinish(p, p->pOut);
     }
   }else if( p->pOut ){
-    sqlite3_free(sqlite3_str_finish(p->pOut));
+    sqlite3_str_free(p->pOut);
   }
   if( p->expMode>0 ){
     sqlite3_stmt_explain(p->pStmt, p->expMode-1);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__attribute__`
- 外部函数：`format`
- 外部函数：`memmove`
- 外部函数：`qrfDigit`
- 外部函数：`qrfSpace`
- 外部函数：`sqlite3_errstr`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_errcode`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_length`
- 外部函数：`sqlite3_str_reset`
- 外部函数：`sqlite3_str_value`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`xWrite`
- 大写宏：`NULL`
- 外部类型：`Begin`
- 外部类型：`Bytes`
- 外部类型：`Convert`
- 外部类型：`Copy`
- 外部类型：`Count`
- 外部类型：`Early`
- 外部类型：`Error`
- 外部类型：`Failed`
- 外部类型：`Fast`
- 外部类型：`Find`
- 外部类型：`Loop`
- 外部类型：`Make`
- 外部类型：`Message`
- 外部类型：`Number`
- 外部类型：`QRF_ESC_Ascii`
- 外部类型：`QRF_ESC_Symbol`
- 外部类型：`Qrf`
- 外部类型：`Query`
- 外部类型：`String`
- 外部类型：`Text`
- 外部类型：`The`
- 外部类型：`Where`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e3fbf0c9e5` → 本草稿移入 `cases/defect/auto-sqlite-e3fbf0c9e5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
