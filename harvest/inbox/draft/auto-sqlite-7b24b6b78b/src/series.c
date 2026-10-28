// AUTO-DRAFT from sqlite/sqlite PR #e5796f671ab939a8fcd0b3667931665ca40b030c
#define LARGEST_INT64  ((sqlite3_int64)0x7fffffffffffffffLL)
/* …（同文件无关代码省略）… */
#define SMALLEST_INT64 ((sqlite3_int64)0x8000000000000000LL)
/* …（同文件无关代码省略）… */
static double seriesCeil(double r){ return ceil(r); }
/* …（同文件无关代码省略）… */
  return (sqlite3_int64)r;
}
  // <<< BUG ANCHOR
/*
** This method is called to "rewind" the series_cursor object back
** to the first row of output.  This method is always called at least
/* …（同文件无关代码省略）… */

  /* Narrow the range of iMin and iMax (the minimum and maximum outputs)
  ** based on equality and inequality constraints on the "value" column.
  */
  if( idxNum & 0x3380 ){
    if( idxNum & 0x0080 ){    /* value=X */
      if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
        double r = sqlite3_value_double(argv[iArg++]);
        if( r==seriesCeil(r)
         && r>=(double)SMALLEST_INT64
/* …（同文件无关代码省略）… */
      }
    }else{
      if( idxNum & 0x0300 ){  /* value>X (0x200) or value>=X (0x100) */
        if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
          double r = sqlite3_value_double(argv[iArg++]);
          if( r<=(double)SMALLEST_INT64 ){
            iMin = SMALLEST_INT64;
/* …（同文件无关代码省略）… */
        }
      }
      if( idxNum & 0x3000 ){   /* value<X (0x2000) or value<=X (0x1000) */
        if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
          double r = sqlite3_value_double(argv[iArg++]);
          if( r>=(double)LARGEST_INT64 ){
            iMax = LARGEST_INT64;
