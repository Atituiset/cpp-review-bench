# auto-postgres-f7812aa221

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#8cdf3c9214feb48179a3cc0fbb0bae0e0f38f2cb](https://github.com/postgres/postgres/commit/8cdf3c9214feb48179a3cc0fbb0bae0e0f38f2cb) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8cdf3c9214feb48179a3cc0fbb0bae0e0f38f2cb (https://github.com/postgres/postgres/commit/8cdf3c9214feb48179a3cc0fbb0bae0e0f38f2cb)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 12（原始 PR diff 行 4064；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8cdf3c9214feb48179a3cc0fbb0bae0e0f38f2cb Fix Memoize using the wrong equality operator for cache keys :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4040,12 +4040,25 @@ check_hashjoinable(RestrictInfo *restrictinfo)
  *	  If the restrictinfo's clause is suitable to be used for a Memoize node,
  *	  set the left_hasheqoperator and right_hasheqoperator to the hash equality
  *	  operator that will be needed during caching.
+ *
+ * When the clause is hashjoinable, the cache must use the same notion of
+ * equality as the join operator, as the type's default equality operator
+ * could consider values equal that the join operator does not.  If both
+ * inputs are of the same type, we can just use the join operator itself;
+ * otherwise we look up the same-type equality operators in the join
+ * operator's hash opfamily, and if there are none, Memoize won't be used.
+ * When the clause is not hashjoinable, we use the type's default equality
+ * operator and Memoize uses binary mode.
+ *
+ * Note: check_hashjoinable() must be called on the given RestrictInfo before
+ * calling this function.
  */
 static void
 check_memoizable(RestrictInfo *restrictinfo)
 {
 	TypeCacheEntry *typentry;
 	Expr	   *clause = restrictinfo->clause;
+	Oid			hashop = restrictinfo->hashjoinoperator;
 	Oid			lefttype;
 	Oid			righttype;
 
@@ -4057,23 +4070,37 @@ check_memoizable(RestrictInfo *restrictinfo)
 		return;
 
 	lefttype = exprType(linitial(((OpExpr *) clause)->args));
-
-	typentry = lookup_type_cache(lefttype, TYPECACHE_HASH_PROC |
-								 TYPECACHE_EQ_OPR);
-
-	if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
-		restrictinfo->left_hasheqoperator = typentry->eq_opr;
-
 	righttype = exprType(lsecond(((OpExpr *) clause)->args));
 
-	/*
-	 * Lookup the right type, unless it's the same as the left type, in which
-	 * case typentry is already pointing to the required TypeCacheEntry.
-	 */
-	if (lefttype != righttype)
-		typentry = lookup_type_cache(righttype, TYPECACHE_HASH_PROC |
+	if (OidIsValid(hashop))
+	{
+		if (lefttype == righttype)
+			restrictinfo->left_hasheqoperator =
+				restrictinfo->right_hasheqoperator = hashop;
+		else
+			(void) get_compatible_hash_operators(restrictinfo->hashjoinoperator,
+												 &restrictinfo->left_hasheqoperator,
+												 &restrictinfo->right_hasheqoperator);
+	}
+	else
+	{
+		/* No hashjoinoperator.  Memoize will use binary mode. */
+		typentry = lookup_type_cache(lefttype, TYPECACHE_HASH_PROC |
 									 TYPECACHE_EQ_OPR);
 
-	if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
-		restrictinfo->right_hasheqoperator = typentry->eq_opr;
+		if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
+			restrictinfo->left_hasheqoperator = typentry->eq_opr;
+
+		/*
+		 * Lookup the right type, unless it's the same as the left type, in
+		 * which case typentry is already pointing to the required
+		 * TypeCacheEntry.
+		 */
+		if (lefttype != righttype)
+			typentry = lookup_type_cache(righttype, TYPECACHE_HASH_PROC |
+										 TYPECACHE_EQ_OPR);
+
+		if (OidIsValid(typentry->hash_proc) && OidIsValid(typentry->eq_opr))
+			restrictinfo->right_hasheqoperator = typentry->eq_opr;
+	}
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`exprType`
- 外部函数：`linitial`
- 外部函数：`lookup_type_cache`
- 外部函数：`lsecond`
- 大写宏：`TYPECACHE_EQ_OPR`
- 大写宏：`TYPECACHE_HASH_PROC`
- 外部类型：`Expr`
- 外部类型：`If`
- 外部类型：`Lookup`
- 外部类型：`Memoize`
- 外部类型：`Oid`
- 外部类型：`OpExpr`
- 外部类型：`RestrictInfo`
- 外部类型：`TypeCacheEntry`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f7812aa221` → 本草稿移入 `cases/defect/auto-postgres-f7812aa221/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
