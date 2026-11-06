// AUTO-DRAFT from sqlite/sqlite PR #b02c93660a3a415f5b8e5c39e64ab4acda97b6c1
** Schema of the terms table.
*/
#define FTS3_AUX_SCHEMA \
  "CREATE TABLE x(term, col, documents, occurrences, languageid HIDDEN)"  // <<< BUG ANCHOR

/*
** This function does all the work for both the xConnect and xCreate methods.
