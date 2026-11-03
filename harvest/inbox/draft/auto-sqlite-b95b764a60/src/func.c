// AUTO-DRAFT from sqlite/sqlite PR #696155ad01f8cbf35b994b7a3d15b00f880d910f
      ** that cannot be converted to a numeric value.
      */
      double rVal = sqlite3_value_double(argv[0]);
      if( rVal<0 ) rVal = -rVal;  // <<< BUG ANCHOR
      sqlite3_result_double(context, rVal);
      break;
    }
/* …（同文件无关代码省略）… */
  */
  if( r<-4503599627370496.0 || r>+4503599627370496.0 ){
    /* The value has no fractional part so there is nothing to round */
  }else if( n==0 ){ 
    r = (double)((sqlite_int64)(r+(r<0?-0.5:+0.5)));
  }else{
    zBuf = sqlite3_mprintf("%!.*f",(int)n,r);
    if( zBuf==0 ){
      sqlite3_result_error_nomem(context);
      return;
