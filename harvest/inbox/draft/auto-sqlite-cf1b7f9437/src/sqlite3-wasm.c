// AUTO-DRAFT from sqlite/sqlite PR #e7c03cb8a135a23e9959ba052531f14c65a7ef88
** DB instance pointing to a stale pointer. Ditto for oo1.DB.Stmt.
*/
#define SQLITE_DISPOSAL_NOTIFIER(T,P) \
  if( (P) && (T==1||T==2) && sqlite3WasmDisposalNotifier.callback ) \  // <<< BUG ANCHOR
    sqlite3WasmDisposalNotifier.callback(T, P)

/*
