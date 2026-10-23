# auto-sqlite-bf3dff20e5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#7c2cb02205d8b36425114ded5fd8deccf27b13f4](https://github.com/sqlite/sqlite/commit/7c2cb02205d8b36425114ded5fd8deccf27b13f4) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 30 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7c2cb02205d8b36425114ded5fd8deccf27b13f4 (https://github.com/sqlite/sqlite/commit/7c2cb02205d8b36425114ded5fd8deccf27b13f4)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2484；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7c2cb02205d8b36425114ded5fd8deccf27b13f4 Improve ALTER TABLE DROP CONSTRAINT so that it is better able to deal :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2444,15 +2444,19 @@ static int getWhitespace(const u8 *z){
 ** keyword.
 **
 ** Argument eTok may be the token value of the first token of the constraint
-** (e.g. TK_CHECK or TK_REFERENCES) or zero. If it is either TK_REFERENCES
-** or TK_FOREIGN, special parsing is enabled to find the end of the foreign-key
-** constraint definition.
+** (e.g. TK_CHECK or TK_REFERENCES or TK_NOT) or zero. If it is either
+** TK_REFERENCES or TK_FOREIGN, special parsing is enabled to find the
+** end of the foreign-key constraint definition.  If eTok is TK_NOT then
+** special processing is enabled to find the end of a NOT NULL constraint
+** including its optional ON CONFLICT clause.
 **
 ** Return the number of bytes until the end of the constraint. 
 */
 static int getConstraint(const u8 *z, int eTok){
   int iOff = 0;
   int t = 0;
+  int t2 = 0;
+  int prev = 0;
 
 #ifndef SQLITE_OMIT_FOREIGN_KEY
   if( eTok==TK_FOREIGN ){
@@ -2468,6 +2472,14 @@ static int getConstraint(const u8 *z, int eTok){
     /* REFERENCES is followed by a table name. Gobble this up here in
     ** case the table name is a fallback token like TK_GENERATED. */
     iOff += getConstraintToken(&z[iOff], &t);
+
+    /* If the table names is followed by a parenthesized list of columns,
+    ** also skip that, lest the close ")" end the constraint in the
+    ** while loop below. */
+    iOff += getWhitespace(&z[iOff]);
+    if( z[iOff]=='(' ){
+      iOff += getConstraintToken(&z[iOff], &t);
+    }
   }
 #endif
 
@@ -2478,17 +2490,34 @@ static int getConstraint(const u8 *z, int eTok){
   **   COLLATE, REFERENCES, FOREIGN, GENERATED, AS, RP, or COMMA
   **
   ** Also exit the loop if ILLEGAL turns up.
+  **
+  ** Exceptions:
+  **
+  **   *  For NOT NULL constraints, also terminate at DEFERRABLE
+  **
+  **   *  Do not terminate at DEFAULT if the previous token was SET
   */
   while( 1 ){
     int n = getConstraintToken(&z[iOff], &t);
-    if( t==TK_CONSTRAINT || t==TK_PRIMARY || t==TK_NOT || t==TK_UNIQUE
-     || t==TK_CHECK || t==TK_DEFAULT || t==TK_COLLATE || t==TK_REFERENCES
+    if( t==TK_CONSTRAINT || t==TK_PRIMARY || t==TK_UNIQUE
+     || t==TK_CHECK || t==TK_COLLATE || t==TK_REFERENCES
+     || (t==TK_DEFAULT && prev!=TK_SET)
      || t==TK_FOREIGN || t==TK_RP || t==TK_COMMA || t==TK_ILLEGAL
      || t==TK_AS || t==TK_GENERATED
+     || (eTok==TK_NOT && t==TK_DEFERRABLE)
     ){
       break;
     }
+    if( t==TK_NOT
+     && (eTok!=TK_REFERENCES
+         || (getConstraintToken(&z[iOff+n],&t2), t2!=TK_DEFERRABLE))
+    ){
+      /* NOT ends the constraint unless eTok is TK_REFERENCES and the next
+      ** token is DEFERRABLE */
+      break;
+    }
     iOff += n;
+    prev = t;
   }
   
   return iOff;
@@ -2655,8 +2684,12 @@ static void dropConstraintFunc(
         }
 
       }else if( t==TK_NOT && iNotNull==ii ){
-        iEnd = iOff + getConstraint(&zSql[iOff], 0);
-        break;
+        int t2;
+        (void)getConstraintToken(zSql+iOff, &t2);
+        if( t2==TK_NULL ){
+          iEnd = iOff + getConstraint(zSql+iOff, TK_NOT);
+          break;
+        }
       }else if( t==TK_RP || t==TK_ILLEGAL ){
         iEnd = -1;
         break;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3GetToken`
- 大写宏：`COLLATE`
- 大写宏：`COMMA`
- 大写宏：`FOREIGN`
- 大写宏：`GENERATED`
- 大写宏：`ILLEGAL`
- 大写宏：`REFERENCES`
- 大写宏：`SQLITE_OMIT_FOREIGN_KEY`
- 大写宏：`TK_AS`
- 大写宏：`TK_CHECK`
- 大写宏：`TK_COLLATE`
- 大写宏：`TK_COMMA`
- 大写宏：`TK_COMMENT`
- 大写宏：`TK_CONSTRAINT`
- 大写宏：`TK_DEFAULT`
- 大写宏：`TK_FOREIGN`
- 大写宏：`TK_GENERATED`
- 大写宏：`TK_ILLEGAL`
- 大写宏：`TK_LP`
- 大写宏：`TK_NOT`
- 大写宏：`TK_PRIMARY`
- 大写宏：`TK_REFERENCES`
- 大写宏：`TK_RP`
- 大写宏：`TK_SPACE`
- 大写宏：`TK_UNIQUE`
- 外部类型：`Also`
- 外部类型：`Argument`
- 外部类型：`Gobble`
- 外部类型：`If`
- 外部类型：`Return`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-bf3dff20e5` → 本草稿移入 `cases/defect/auto-sqlite-bf3dff20e5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
