# auto-sqlite-a157d856f7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#5e19f7e4bb5d37afa41eaa30f0334721ec3aae41](https://github.com/sqlite/sqlite/commit/5e19f7e4bb5d37afa41eaa30f0334721ec3aae41) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5e19f7e4bb5d37afa41eaa30f0334721ec3aae41 (https://github.com/sqlite/sqlite/commit/5e19f7e4bb5d37afa41eaa30f0334721ec3aae41)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5e19f7e4bb5d37afa41eaa30f0334721ec3aae41 Speed up substr() start scanning for ASCII text :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -333,6 +333,14 @@ static void printfFunc(
   }
 }
 
+/* Return true if any of the eight bytes in word is NUL or non-ASCII. */
+static SQLITE_INLINE int containsNulOrNonAscii(u64 word){
+  const u64 byteOnes = UINT64_C(0x0101010101010101);
+  const u64 byteHighBits = UINT64_C(0x8080808080808080);
+  /* NUL causes a borrow into a high bit; non-ASCII already has one set. */
+  return ((word | (word - byteOnes)) & byteHighBits)!=0;
+}
+
 /*
 ** Implementation of the substr() function.
 **
@@ -416,6 +424,20 @@ static void substrFunc(
   }
   assert( p1>=0 && p2>=0 );
   if( p0type!=SQLITE_BLOB ){
+    if( p1>=8 ){
+      /* Skip 8 characters at a time while all 8 bytes are in 0x01..0x7f */
+      const unsigned char *zEnd = z + sqlite3_value_bytes(argv[0]);
+      while( p1>=8 && zEnd-z>=8 ){
+        u64 word;
+        memcpy(&word, z, sizeof(word));
+        /* Handle NUL and UTF-8 with the scalar loop below. */
+        if( containsNulOrNonAscii(word) ){
+          break;
+        }
+        z += 8;
+        p1 -= 8;
+      }
+    }
     for( ; p1>0; p1--){
                     /*  vvvvvv----  See tag-20260418-01 */
       if( (u8)(z[0]-1)<(0x80-1) ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`substr`
- 大写宏：`SQLITE_BLOB`
- 外部类型：`Implementation`
- 外部类型：`See`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-a157d856f7` → 本草稿移入 `cases/defect/auto-sqlite-a157d856f7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
