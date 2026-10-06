# auto-sqlite-9f72a273dc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#95170adc372e2a3d7ca664c3f3673a5258b0dd78](https://github.com/sqlite/sqlite/commit/95170adc372e2a3d7ca664c3f3673a5258b0dd78) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #95170adc372e2a3d7ca664c3f3673a5258b0dd78 (https://github.com/sqlite/sqlite/commit/95170adc372e2a3d7ca664c3f3673a5258b0dd78)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 18（原始 PR diff 行 218；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 95170adc372e2a3d7ca664c3f3673a5258b0dd78 Fixes to sqlite3_incomplete() so that it does not appear to presume a :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -215,7 +215,6 @@ sqlite3_int64 sqlite3_incomplete(const char *zSql){
         zSql += 2;
         while( zSql[0] && (zSql[0]!='*' || zSql[1]!='/') ){ zSql++; }
         if( zSql[0]==0 ){
-          if( state==0 ) state = 2;
           pending = '/';
           goto incomplete_finish;
         }
@@ -230,7 +229,7 @@ sqlite3_int64 sqlite3_incomplete(const char *zSql){
         }
         while( *zSql && *zSql!='\n' ){ zSql++; }
         if( *zSql==0 ){
-          if( state!=1 ) pending = '-';
+          pending = '-';
           goto incomplete_finish;
         }
         token = tkWS;
@@ -333,12 +332,12 @@ sqlite3_int64 sqlite3_incomplete(const char *zSql){
     zSql++;
   }
 incomplete_finish:
-  if( state==0 ) return SQLITE_EMPTY;
+  if( state==0 && pending==0 ) return SQLITE_EMPTY;
   if( state==1 ) nParen = 0;
   return (i64)((((u64)nParen)<<32) |
                ((u64)pending<<16) |
                ((u64)statemap[state]<<8) |
-               (state!=1));
+               (state>1 || pending!=0));
 }
 int sqlite3_complete(const char *zSql){
   return sqlite3_incomplete(zSql)==0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3StrNICmp`
- 大写宏：`CREATE`
- 大写宏：`END`
- 大写宏：`EXPLAIN`
- 大写宏：`NORMAL`
- 大写宏：`NOTHING`
- 大写宏：`OTHER`
- 大写宏：`SEMI`
- 大写宏：`SQL`
- 大写宏：`SQLITE_EBCDIC`
- 大写宏：`SQLITE_ENABLE_API_ARMOR`
- 大写宏：`SQLITE_MISUSE_BKPT`
- 大写宏：`SQLITE_OMIT_EXPLAIN`
- 大写宏：`SQLITE_OMIT_TRIGGER`
- 大写宏：`START`
- 大写宏：`TEMP`
- 大写宏：`TRIGGER`
- 外部类型：`Current`
- 外部类型：`Grave`
- 外部类型：`If`
- 外部类型：`Keywords`
- 外部类型：`Mapping`
- 外部类型：`Microsoft`
- 外部类型：`MySQL`
- 外部类型：`Nested`
- 外部类型：`Operators`
- 外部类型：`State`
- 外部类型：`This`
- 外部类型：`Token`
- 外部类型：`Value`
- 外部类型：`White`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-9f72a273dc` → 本草稿移入 `cases/defect/auto-sqlite-9f72a273dc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
