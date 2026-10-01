// AUTO-DRAFT from redis/redis PR #15034
void backgroundSaveDoneHandler(int exitcode, int bysignal);
int rdbSaveKeyValuePair(rio *rdb, robj *key, robj *val, long long expiretime,int dbid);
ssize_t rdbSaveSingleModuleAux(rio *rdb, int when, moduleType *mt);
robj *rdbLoadCheckModuleValue(rio *rdb, char *modulename);  // <<< BUG ANCHOR
int rdbResolveKeyType(rio *rdb, int *type, int dbid, KeyMetaSpec *keymeta);
robj *rdbLoadStringObject(rio *rdb);
ssize_t rdbSaveStringObject(rio *rdb, robj *obj);
