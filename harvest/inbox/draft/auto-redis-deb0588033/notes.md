# auto-redis-deb0588033

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15519 (https://github.com/redis/redis/pull/15519)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 155；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -152,7 +152,6 @@ long long rdbLoadMillisecondTime(rio *rdb, int rdbver);
 uint64_t rdbLoadLen(rio *rdb, int *isencoded);
 int rdbLoadLenByRef(rio *rdb, int *isencoded, uint64_t *lenptr);
 int rdbSaveObjectType(rio *rdb, robj *o);
-int rdbSaveSetRefMode(int enable);
 int rdbLoadObjectType(rio *rdb);
 int rdbLoadWithEmptyFunc(char *filename, rdbSaveInfo *rsi, int rdbflags, void (*emptyDbFunc)(void));
 int rdbLoad(char *filename, rdbSaveInfo *rsi, int rdbflags);
@@ -165,7 +164,7 @@ ssize_t rdbSaveObject(rio *rdb, robj *o, robj *key, int dbid);
 size_t rdbSavedObjectLen(robj *o, robj *key, int dbid);
 robj *rdbLoadObject(int rdbtype, rio *rdb, sds key, int dbid, int *error);
 void backgroundSaveDoneHandler(int exitcode, int bysignal);
-int rdbSaveKeyValuePair(rio *rdb, robj *key, robj *val, long long expiretime,int dbid);
+int rdbSaveKeyValuePair(rio *rdb, robj *key, robj *val, long long expiretime, int dbid);
 ssize_t rdbSaveSingleModuleAux(rio *rdb, int when, moduleType *mt);
 robj *rdbLoadCheckModuleValue(rio *rdb, char *modulename, int null_on_error);
 int rdbResolveKeyType(rio *rdb, int *type, int dbid, KeyMetaSpec *keymeta);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-deb0588033` → 本草稿移入 `cases/defect/auto-redis-deb0588033/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
