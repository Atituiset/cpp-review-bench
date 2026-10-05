# auto-sqlite-1685ba24f3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e8cbe2c96a69b2c674947f2efff4c7d1ada8b66e](https://github.com/sqlite/sqlite/commit/e8cbe2c96a69b2c674947f2efff4c7d1ada8b66e) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e8cbe2c96a69b2c674947f2efff4c7d1ada8b66e (https://github.com/sqlite/sqlite/commit/e8cbe2c96a69b2c674947f2efff4c7d1ada8b66e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 3031；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e8cbe2c96a69b2c674947f2efff4c7d1ada8b66e In QRF, add the zero-pad flag to the default floating-point format in :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2893,6 +2893,9 @@ static void qrfInitialize(
   if( p->spec.eTitle>QRF_TEXT_Relaxed ) p->spec.eTitle = QRF_Auto;
   if( p->spec.eBlob>QRF_BLOB_Size ) p->spec.eBlob = QRF_Auto;
 qrf_reinit:
+  if( p->spec.zFpFmt && sqlite3_qrf_ckformat(p->spec.zFpFmt)!=2 ){
+    p->spec.zFpFmt = 0;
+  }
   switch( p->spec.eStyle ){
     case QRF_Auto: {
       switch( sqlite3_stmt_isexplain(pStmt) ){
@@ -2911,6 +2914,7 @@ static void qrfInitialize(
     case QRF_STYLE_Json: {
       p->spec.eText = QRF_TEXT_Json;
       p->spec.zNull = "null";
+      if( p->spec.zFpFmt==0 ) p->spec.zFpFmt = "%0.16g";
       break;
     }
     case QRF_STYLE_Html: {
@@ -2924,6 +2928,7 @@ static void qrfInitialize(
       if( p->spec.zTableName==0 || p->spec.zTableName[0]==0 ){
         p->spec.zTableName = "tab";
       }
+      if( p->spec.zFpFmt==0 ) p->spec.zFpFmt = "%0.16g";
       p->u.nIns = 0;
       break;
     }
@@ -3028,9 +3033,6 @@ static void qrfInitialize(
     p->zFmt[n+2] = 0;
     p->spec.zIFmt = p->zFmt;
   }
-  if( p->spec.zFpFmt && sqlite3_qrf_ckformat(p->spec.zFpFmt)!=2 ){
-    p->spec.zFpFmt = 0;
-  }
 }
 
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_stmt_isexplain`
- 外部函数：`strspn`
- 外部类型：`QRF_Auto`
- 外部类型：`QRF_BLOB_Size`
- 外部类型：`QRF_STYLE_Html`
- 外部类型：`QRF_STYLE_Json`
- 外部类型：`QRF_TEXT_Json`
- 外部类型：`QRF_TEXT_Relaxed`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-1685ba24f3` → 本草稿移入 `cases/defect/auto-sqlite-1685ba24f3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
