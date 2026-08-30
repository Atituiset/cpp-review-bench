// AUTO-DRAFT from redis/redis PR #15519
uint64_t rdbLoadLen(rio *rdb, int *isencoded);
int rdbLoadLenByRef(rio *rdb, int *isencoded, uint64_t *lenptr);
int rdbSaveObjectType(rio *rdb, robj *o);
int rdbSaveSetRefMode(int enable);  // <<< BUG ANCHOR
int rdbLoadObjectType(rio *rdb);
int rdbLoadWithEmptyFunc(char *filename, rdbSaveInfo *rsi, int rdbflags, void (*emptyDbFunc)(void));
int rdbLoad(char *filename, rdbSaveInfo *rsi, int rdbflags);
size_t rdbSavedObjectLen(robj *o, robj *key, int dbid);
robj *rdbLoadObject(int rdbtype, rio *rdb, sds key, int dbid, int *error);
void backgroundSaveDoneHandler(int exitcode, int bysignal);
int rdbSaveKeyValuePair(rio *rdb, robj *key, robj *val, long long expiretime,int dbid);
ssize_t rdbSaveSingleModuleAux(rio *rdb, int when, moduleType *mt);
robj *rdbLoadCheckModuleValue(rio *rdb, char *modulename, int null_on_error);
int rdbResolveKeyType(rio *rdb, int *type, int dbid, KeyMetaSpec *keymeta);
