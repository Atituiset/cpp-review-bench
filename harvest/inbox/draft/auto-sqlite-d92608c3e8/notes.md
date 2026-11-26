# auto-sqlite-d92608c3e8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#915d9c4acea4a9036d1df7dcf9e8bd747ba726c4](https://github.com/sqlite/sqlite/commit/915d9c4acea4a9036d1df7dcf9e8bd747ba726c4) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 37（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #915d9c4acea4a9036d1df7dcf9e8bd747ba726c4 (https://github.com/sqlite/sqlite/commit/915d9c4acea4a9036d1df7dcf9e8bd747ba726c4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1281；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 915d9c4acea4a9036d1df7dcf9e8bd747ba726c4 BLOB output from the CLI under ".mode html" is interpreted as text under the :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1272,22 +1272,29 @@ static void qrfRenderValue(Qrf *p, sqlite3_str *pOut, int iCol){
           break;
         }
         default: {
-          const void *pBlob = sqlite3_column_blob(p->pStmt,iCol);
-          int nBlob = sqlite3_column_bytes(p->pStmt,iCol);
-          int rc;
-          qrfWrite(p);
-          if( nBlob==0 ){
-            /* no-op */
-          }else if( p->spec.eEsc==QRF_ESC_Off ){
-            rc = p->spec.xWrite(p->spec.pWriteArg,pBlob,nBlob);
-            if( rc ){
-              qrfError(p, rc, "Failed to write %d bytes of BLOB output", nBlob);
-            }
+          if( p->spec.eBlob==QRF_BLOB_Auto && p->spec.eStyle==QRF_STYLE_Html ){
+            const char *zTxt = (const char*)sqlite3_column_text(p->pStmt,iCol);
+            qrfEncodeText(p, pOut, zTxt);
           }else{
-            sqlite3_str_append(pOut, pBlob, nBlob);
-            qrfEscape(p->spec.eEsc, pOut, 0);
+            const void *pBlob = sqlite3_column_blob(p->pStmt,iCol);
+            int nBlob = sqlite3_column_bytes(p->pStmt,iCol);
+            int rc;
             qrfWrite(p);
+            if( nBlob==0 ){
+              /* no-op */
+            }else if( p->spec.eEsc==QRF_ESC_Off ){
+              rc = p->spec.xWrite(p->spec.pWriteArg,pBlob,nBlob);
+              if( rc ){
+                qrfError(p, rc, "Failed to write %d bytes of BLOB output",
+                         nBlob);
+              }
+            }else{
+              sqlite3_str_append(pOut, pBlob, nBlob);
+              qrfEscape(p->spec.eEsc, pOut, 0);
+              qrfWrite(p);
+            }
           }
+          break;
         }
       }
       break;
@@ -3084,6 +3091,7 @@ static void qrfInitialize(
       case QRF_TEXT_Csv:  p->spec.eBlob = QRF_BLOB_Tcl;  break;
       case QRF_TEXT_Tcl:  p->spec.eBlob = QRF_BLOB_Tcl;  break;
       case QRF_TEXT_Json: p->spec.eBlob = QRF_BLOB_Json; break;
+      case QRF_TEXT_Html: p->spec.eBlob = QRF_BLOB_Auto; break;
       default:            p->spec.eBlob = QRF_BLOB_Text; break;
     }
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`format`
- 外部函数：`memmove`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendchar`
- 外部函数：`sqlite3_str_length`
- 外部函数：`sqlite3_str_reset`
- 外部函数：`sqlite3_str_value`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`xWrite`
- 大写宏：`BLOB`
- 大写宏：`NULL`
- 外部类型：`Begin`
- 外部类型：`Bytes`
- 外部类型：`Convert`
- 外部类型：`Copy`
- 外部类型：`Count`
- 外部类型：`Early`
- 外部类型：`Error`
- 外部类型：`Failed`
- 外部类型：`Find`
- 外部类型：`Loop`
- 外部类型：`Make`
- 外部类型：`Message`
- 外部类型：`Number`
- 外部类型：`QRF_BLOB_Json`
- 外部类型：`QRF_BLOB_Tcl`
- 外部类型：`QRF_BLOB_Text`
- 外部类型：`QRF_ESC_Ascii`
- 外部类型：`QRF_ESC_Off`
- 外部类型：`QRF_ESC_Symbol`
- 外部类型：`QRF_TEXT_Csv`
- 外部类型：`QRF_TEXT_Json`
- 外部类型：`QRF_TEXT_Tcl`
- 外部类型：`Qrf`
- 外部类型：`Query`
- 外部类型：`String`
- 外部类型：`Text`
- 外部类型：`Where`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-d92608c3e8` → 本草稿移入 `cases/defect/auto-sqlite-d92608c3e8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
