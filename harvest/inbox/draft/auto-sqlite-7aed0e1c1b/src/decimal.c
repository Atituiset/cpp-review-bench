// AUTO-DRAFT from sqlite/sqlite PR #623bc0b1d5e5a45236c8d2f46f52f96dfe90b780
  sqlite3_value *pVal,
  int *pResult
){  // <<< BUG ANCHOR
  sqlite3_int64 i64 = sqlite3_value_int64(pVal);
  if( i64<-2147483648LL || i64>2147483647LL ){
    char *zMsg = sqlite3_mprintf(
      "value %lld is too large: range limit -2147483648..+2147483647",
      i64);
    sqlite3_result_error(context, zMsg, -1);
    sqlite3_free(zMsg);
    *pResult = 0;
    return 1;
  }else{
    *pResult = (int)i64;
    return 0;
  }
}
