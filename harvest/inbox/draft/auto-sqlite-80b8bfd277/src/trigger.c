// AUTO-DRAFT from sqlite/sqlite PR #53c0bc98ac671f972cc4522a5007eb27ecc84e3c
  ** then set iDb to 1 to create the trigger in the temporary database.
  ** If sqlite3SrcListLookup() returns 0, indicating the table does not
  ** exist, the error is caught by the block below.
  */
  pTab = sqlite3SrcListLookup(pParse, pTableName);
  if( db->init.busy==0 && pName2->n==0 && pTab  // <<< BUG ANCHOR
        && pTab->pSchema==db->aDb[1].pSchema ){
    iDb = 1;
  }

  /* Ensure the table name matches database name and that the table exists */
