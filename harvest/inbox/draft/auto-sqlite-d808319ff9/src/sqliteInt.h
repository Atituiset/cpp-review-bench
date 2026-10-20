// AUTO-DRAFT from sqlite/sqlite PR #17f128f7466f21396e05591067c36ad8dbbb0f3d
typedef struct ParseCleanup ParseCleanup;
/* …（同文件无关代码省略）… */
typedef struct Table Table;
/* …（同文件无关代码省略）… */
typedef struct TriggerPrg TriggerPrg;
/* …（同文件无关代码省略）… */
  Table *pTriggerTab;  /* Table triggers are being coded for */
  TriggerPrg *pTriggerPrg;  /* Linked list of coded triggers */
  ParseCleanup *pCleanup;   /* List of cleanup operations to run after parse */

  /**************************************************************************
  ** Fields above must be initialized to zero.  The fields that follow,
