# auto-sqlite-5e102f4a14

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#edb49992bb532644f696f8e488a51860acf9439a](https://github.com/sqlite/sqlite/commit/edb49992bb532644f696f8e488a51860acf9439a) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 38 |
| 编译错误数（gcc syntax-only） | 44（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #edb49992bb532644f696f8e488a51860acf9439a (https://github.com/sqlite/sqlite/commit/edb49992bb532644f696f8e488a51860acf9439a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2421；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT edb49992bb532644f696f8e488a51860acf9439a Enhance the JSON interface so that it will accept integer literals :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2358,6 +2358,94 @@ static SQLITE_INLINE u32 jsonbPayloadSizeSemiInline(
 # define jsonbPayloadSizeSemiInline(a,b,c) jsonbPayloadSize(a,b,c)
 #endif
 
+/*
+** Convert a JSONB integer literal into either an integer (if it is small
+** enough) or into a double.  Include the sign.  Return:
+**
+**     0     Value is an integer, results written into *pI.
+**     1     Value is a double, results written into *pR.
+**     2     Syntax error
+*/
+static int jsonConvertInteger(const char *zIn, u32 sz, i64 *pI, double *pR){
+  u32 k = 0;
+  int bNeg = 0;
+  sqlite3_uint64 u = 0;
+  double rOverflow = 1.0;
+  if( sz==0 ) return 2;
+  if( zIn[0]=='-' ){
+    bNeg = 1;
+    k = 1;
+  }else if( zIn[0]=='+' ){
+    k = 1;
+  }
+  if( k+2<sz && zIn[k]=='0' && (zIn[k+1]=='x' || zIn[k+1]=='X') ){
+    k += 2;
+    do{
+      if( !sqlite3Isxdigit(zIn[k]) ){
+        return 2;
+      }else if( (u>>60)!=0 ){
+        rOverflow *= 16.0;
+      }else{
+        u = u*16 + sqlite3HexToInt(zIn[k]);
+      }
+      k++;
+    }while( k<sz );
+  }else if( k>=sz ){
+    return 2;
+  }else{
+    do{
+      if( zIn[k]<'0' || zIn[k]>'9' ){
+        return 2;
+      }else if( (u>>60)!=0 ){
+        rOverflow *= 10.0;
+      }else{
+        u = u*10 + zIn[k] - '0';
+      }
+      k++;
+    }while( k<sz );
+  }
+  if( rOverflow>1.0
+   || (u>0x7fffffffffffffffULL && (!bNeg || u>0x8000000000000000ULL))
+  ){
+    *pR = rOverflow * (double)u;
+    if( bNeg ) *pR = -*pR;
+    return 1;
+  }else{
+    memcpy(pI, &u, sizeof(u));
+    if( bNeg ) *pI = -*pI;
+    return 0;
+  }
+}
+
+/*
+** Tail recursive code to implement the JSON_INT5 case of of
+** jsonTranslateBlobInt5ToText().  This is split out because
+** the resulting reduction in the stack frame size helps the
+** caller to run faster.
+*/
+static SQLITE_NOINLINE u32 jsonTranslateBlobInt5ToText(
+  JsonParse *pParse,             /* the complete parse of the JSON */
+  u32 i,                         /* Start rendering at this index */
+  JsonString *pOut,              /* Write JSON here */
+  u32 sz,                        /* Payload size */
+  u32 n                          /* Header size */
+){
+  int k;
+  const char *zIn = (const char*)&pParse->aBlob[i+n];
+  i64 ival;
+  double rval;
+  k = jsonConvertInteger(zIn, sz, &ival, &rval);
+  if( k==0 ){
+    jsonPrintf(100, pOut, "%lld", ival);
+  }else if( k==1 ){
+    jsonPrintf(100, pOut, "%!0.17g", rval);
+  }else{
+    pOut->eErr |= JSTRING_MALFORMED;
+  }
+  return i+n+sz;
+}
+
+
 /*
 ** Translate the binary JSONB representation of JSON beginning at
 ** pParse->aBlob[i] into a JSON text string.  Append the JSON
@@ -2403,29 +2491,7 @@ static u32 jsonTranslateBlobToText(
       break;
     }
     case JSONB_INT5: {  /* Integer literal in hexadecimal notation */
-      u32 k = 2;
-      sqlite3_uint64 u = 0;
-      const char *zIn = (const char*)&pParse->aBlob[i+n];
-      int bOverflow = 0;
-      if( sz==0 ) goto malformed_jsonb;
-      if( zIn[0]=='-' ){
-        jsonAppendChar(pOut, '-');
-        k++;
-      }else if( zIn[0]=='+' ){
-        k++;
-      }
-      for(; k<sz; k++){
-        if( !sqlite3Isxdigit(zIn[k]) ){
-          pOut->eErr |= JSTRING_MALFORMED;
-          break;
-        }else if( (u>>60)!=0 ){
-          bOverflow = 1;
-        }else{
-          u = u*16 + sqlite3HexToInt(zIn[k]);
-        }
-      }
-      jsonPrintf(100,pOut,bOverflow?"9.0e999":"%llu", u);
-      break;
+      return jsonTranslateBlobInt5ToText(pParse, i, pOut, sz, n);
     }
     case JSONB_FLOAT5: { /* Float literal missing digits beside "." */
       u32 k = 0;
@@ -3441,43 +3507,16 @@ static void jsonReturnFromBlob(
     }
     case JSONB_INT5:
     case JSONB_INT: {
-      sqlite3_int64 iRes = 0;
-      char *z;
-      int bNeg = 0;
-      char x;
-      if( sz==0 ) goto returnfromblob_malformed;
-      x = (char)pParse->aBlob[i+n];
-      if( x=='-' ){
-        if( sz<2 ) goto returnfromblob_malformed;
-        n++;
-        sz--;
-        bNeg = 1;
-     
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`jsonbPayloadSizeSemiInline`
- 外部函数：`memcpy`
- 外部函数：`sqlite3HexToInt`
- 外部函数：`sqlite3Isxdigit`
- 外部函数：`sqlite3RCStrNew`
- 外部函数：`sqlite3RCStrResize`
- 外部函数：`sqlite3RCStrUnref`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_vsnprintf`
- 外部函数：`strlen`
- 外部函数：`testcase`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`JSON`
- 大写宏：`JSON5`
- 大写宏：`JSONB`
- 大写宏：`NO_TEST`
- 大写宏：`SQL`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 外部类型：`Append`
- 外部类型：`Float`
- 外部类型：`Index`
- 外部类型：`Integer`
- 外部类型：`JsonParse`
- 外部类型：`JsonString`
- 外部类型：`Malformed`
- 外部类型：`Os`
- 外部类型：`Out`
- 外部类型：`Quirks`
- 外部类型：`The`
- 外部类型：`Translate`
- 外部类型：`Write`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-5e102f4a14` → 本草稿移入 `cases/defect/auto-sqlite-5e102f4a14/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
