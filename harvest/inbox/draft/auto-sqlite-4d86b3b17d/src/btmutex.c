// AUTO-DRAFT from sqlite/sqlite PR #b495f75d817c85f3cfcab11dff52b2e531aaf5b9
}
void sqlite3BtreeEnterAll(sqlite3 *db){
  int i;
  for(i=0; i<db->nDb; i++){
    Btree *p = db->aDb[i].pBt;
    if( p ){
      p->pBt->db = p->db;
    }
  }
}
#endif /* if SQLITE_THREADSAFE */
