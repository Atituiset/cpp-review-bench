// AUTO-DRAFT from sqlite/sqlite PR #82fda1d99a7d87cb0bffce0681057d76d295087b
  int rc;
  sqlite3_stmt *pStmt;
  int n;
  sqlite3_int64 ii;
  sqlite3_int64 pgsz;
  sqlite3_int64 nPage;
/* …（同文件无关代码省略）… */
  sqlite3_str_appendf(s.pOut,
    "The following SQL will create a table named \"space_used\" which\n"
    "contains most of the information used to generate the report above.\n"
    "*/\n"  // <<< BUG ANCHOR
  );
  sqlite3_str_appendf(s.pOut,
    "BEGIN;\n"
    "CREATE TABLE space_used(\n"
    "   name text,                -- A table or index\n"                /* 0 */
