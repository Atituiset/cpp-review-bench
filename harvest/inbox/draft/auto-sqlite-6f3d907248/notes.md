# auto-sqlite-6f3d907248

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#a27bcfe21c9b4e0989e86eec8fb34a3832cd23c7](https://github.com/sqlite/sqlite/commit/a27bcfe21c9b4e0989e86eec8fb34a3832cd23c7) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 88 |
| 编译错误数（gcc syntax-only） | 99（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a27bcfe21c9b4e0989e86eec8fb34a3832cd23c7 (https://github.com/sqlite/sqlite/commit/a27bcfe21c9b4e0989e86eec8fb34a3832cd23c7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 11561；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a27bcfe21c9b4e0989e86eec8fb34a3832cd23c7 Fix a vec1 problem where rounding errors when assigning vectors to centroids could cause delete operations to fail. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2044,6 +2044,21 @@ static int vec1BestMatchSimple(
   return iBest;
 }
 
+/*
+** Relative tolerance used by vec1BucketIsNearTie().
+*/
+#define VEC1_BUCKET_TOLERANCE 0.00001
+
+/*
+** fBest is the distance from a vector to its nearest IVF centroid, as
+** found by vec1BestMatchSimple(). fDist is the distance from the same
+** vector to some other centroid. Return true if the two distances are
+** close enough that rounding differences could account for the difference.
+*/
+static int vec1BucketIsNearTie(double fBest, double fDist){
+  return (fDist - fBest) <= (fBest * VEC1_BUCKET_TOLERANCE);
+}
+
 /*
 ** Return a pseudo-random positive 32-bit value.
 */
@@ -11540,6 +11555,70 @@ static int vec1SpecialInsert(
 */
 #define VEC1_TOMBSTONE_THRESHOLD(nEntry) ((nEntry * 2) / 10)
 
+/*
+** Locate the index entry for row iRowid, for writing. Array aVec[] is the
+** row's transformed vector. 
+**
+** The bucket selected for aVec[] by vec1BestMatchSimple() is searched
+** first. If the model has no buckets, this is bucket 0 and it is the only
+** bucket searched. Otherwise, if the row is not found there, it may have
+** been assigned to a different bucket by code that rounds differently -
+** either a different routine, such as the batched routines used by 
+** 'rebuild', or the same routine on a different platform. In that case
+** all other buckets that are near-ties with the selected bucket (see 
+** vec1BucketIsNearTie()) are searched, followed by all remaining buckets.
+**
+** If the row is found, (*pLoc) is populated and (*piBucket) is set to
+** the bucket it was found in. If it is not found, (*pLoc) is zeroed and
+** SQLITE_OK returned. The caller must then treat the row as missing.
+*/
+static int vec1DeleteFindRowid(
+  Vec1Tab *pTab,                  /* Table to search */
+  const float *aVec,              /* Transformed vector */
+  i64 iRowid,                     /* Rowid to locate */
+  int *piBucket,                  /* OUT: Bucket containing iRowid */
+  Vec1RowidLocation *pLoc         /* OUT: Location of iRowid */
+){
+  const Vec1Model *pMod = &pTab->mod;
+  const int nElem = (int)pMod->hdr.nElem;
+  const int nBucket = (int)pMod->hdr.nBucket;
+  int iBest = 0;
+  double fBest = 0.0;
+  int rc = SQLITE_OK;
+
+  int iPass;
+
+  if( nBucket>0 ){
+    iBest = vec1BestMatchSimple(pMod->aCentroid, nBucket, aVec, nElem, &fBest);
+  }
+  rc = vec1FindByRowid(pTab, iBest, 1, iRowid, pLoc);
+  if( rc!=SQLITE_OK || pLoc->pList || pLoc->pBlob ){
+    *piBucket = iBest;
+    return rc;
+  }
+
+  /* Pass 0 searches buckets that are near-ties with iBest. Pass 1 searches
+  ** all others.  */
+  for(iPass=0; iPass<2; iPass++){
+    int ii;
+    for(ii=0; ii<nBucket; ii++){
+      if( ii!=iBest ){
+        double fDist;
+        fDist = vec1L2Dist(aVec, &pMod->aCentroid[ii*nElem], nElem);
+        if( vec1BucketIsNearTie(fBest, fDist)==(iPass==0) ){
+          rc = vec1FindByRowid(pTab, ii, 1, iRowid, pLoc);
+          if( rc!=SQLITE_OK || pLoc->pList || pLoc->pBlob ){
+            *piBucket = ii;
+            return rc;
+          }
+        }
+      }
+    }
+  }
+
+  return SQLITE_OK;
+}
+
 /*
 ** Delete the %_base entry for the row with rowid=iRowid. Also zero-out
 ** the corresponding %_idx entry.
@@ -11553,40 +11632,34 @@ static int vec1DeleteByRowid(Vec1Tab *pTab, i64 iRowid){
 
   rc = vec1GetSql(pTab, VEC1_SQL_DEL_LOOKUP_BASE, &pDelete);
   if( rc==SQLITE_OK ){
-    int iBucket = 0;
 
     /* Delete the row from the %_base table. Also set variable iBucket to
-    ** the index of the bucket containing the vector.  */
+    ** the index of the bucket containing the vector. Statement pDelete is
+    ** not reset until the end of this function, as aTransform[] may point
+    ** to the vector it returns.  */
     sqlite3_bind_int64(pDelete, 1, iRowid);
-    if( SQLITE_ROW==sqlite3_step(pDelete) && pTab->mod.hdr.nBucket>0 ){
+    if( (SQLITE_ROW==sqlite3_step(pDelete))
+     && (pTab->mod.hdr.flags & VEC1_MODEL_INDEX)
+    ){
+ 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_mm256_castps256_ps128`
- 外部函数：`_mm256_extractf128_ps`
- 外部函数：`_mm256_fmadd_ps`
- 外部函数：`_mm256_loadu_ps`
- 外部函数：`_mm256_setzero_ps`
- 外部函数：`_mm256_sub_ps`
- 外部函数：`_mm_add_ps`
- 外部函数：`_mm_cvtss_f32`
- 外部函数：`_mm_hadd_ps`
- 外部函数：`assert`
- 外部函数：`length`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`size_of_array`
- 外部函数：`sqlite3_bind_int`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_blob_close`
- 外部函数：`sqlite3_blob_open`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sqrt`
- 外部函数：`sum`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vaddvq_f32`
- 外部函数：`vdupq_n_f32`
- 外部函数：`vec1FloatWithinTolerance`
- 外部函数：`vfmaq_f32`
- 外部函数：`vld1q_f32`
- 外部函数：`vmlaq_f32`
- 外部函数：`vsubq_f32`
- 大写宏：`AND`
- 大写宏：`BETWEEN`
- 大写宏：`CONFLICT`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`INFINITY`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LIMIT`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`REPLACE`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Also`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`Codebook`
- 外部类型：`Create`
- 外部类型：`Delete`
- 外部类型：`Distance`
- 外部类型：`For`
- 外部类型：`Now`
- 外部类型：`Number`
- 外部类型：`Or`
- 外部类型：`Process`
- 外部类型：`Result`
- 外部类型：`Return`
- 外部类型：`Row`
- 外部类型：`Rowid`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`Sum`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`Use`
- 外部类型：`Used`
- 外部类型：`Vec1Buffer`
- 外部类型：`Vec1ListBuilder`
- 外部类型：`Vec1Model`
- 外部类型：`Vec1RowidLocation`
- 外部类型：`Vec1Tab`
- 外部类型：`float32x4_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6f3d907248` → 本草稿移入 `cases/defect/auto-sqlite-6f3d907248/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
