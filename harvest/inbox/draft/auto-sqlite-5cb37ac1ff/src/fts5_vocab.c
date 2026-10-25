// AUTO-DRAFT from sqlite/sqlite PR #e7dfb3cae950e593696f5e58aa216be005ce4fbd
typedef struct Fts5VocabTable Fts5VocabTable;
/* …（同文件无关代码省略）… */
#define FTS5_VOCAB_ROW      1
#define FTS5_VOCAB_INSTANCE 2
  // <<< BUG ANCHOR
#define FTS5_VOCAB_COL_SCHEMA  "term, col, doc, cnt"
#define FTS5_VOCAB_ROW_SCHEMA  "term, doc, cnt"
#define FTS5_VOCAB_INST_SCHEMA "term, doc, col, offset"

/*
** Bits for the mask used as the idxNum value by xBestIndex/xFilter.
/* …（同文件无关代码省略）… */
  char **pzErr                    /* Write any error message here */
){
  const char *azSchema[] = { 
    "CREATE TABlE vocab(" FTS5_VOCAB_COL_SCHEMA  ")", 
    "CREATE TABlE vocab(" FTS5_VOCAB_ROW_SCHEMA  ")",
    "CREATE TABlE vocab(" FTS5_VOCAB_INST_SCHEMA ")"
  };

  Fts5VocabTable *pRet = 0;
