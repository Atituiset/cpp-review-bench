// AUTO-DRAFT from sqlite/sqlite PR #77f8caa273d6d30f3f0e278b2ac04715a87b37df
typedef struct AuxData AuxData;
/* …（同文件无关代码省略）… */
typedef struct VdbeCursor VdbeCursor;
/* …（同文件无关代码省略）… */
  /* ShallowCopy only needs to copy the information above */
  sqlite3 *db;        /* The associated database connection */
  int szMalloc;       /* Size of the zMalloc allocation */
  tSerial uTemp;      /* Transient storage for serial_type in OP_MakeRecord */  // <<< BUG ANCHOR
  char *zMalloc;      /* Space to hold MEM_Str or MEM_Blob if szMalloc>0 */
  void (*xDel)(void*);/* Destructor for Mem.z - only valid if MEM_Dyn */
#ifdef SQLITE_DEBUG
/* …（同文件无关代码省略）… */
int SQLITE_NOINLINE sqlite3VdbeHandleMovedCursor(VdbeCursor *p);
int SQLITE_NOINLINE sqlite3VdbeFinishMoveto(VdbeCursor*);
int sqlite3VdbeCursorRestore(VdbeCursor*);
u32 sqlite3VdbeSerialTypeLen(tSerial);
u8 sqlite3VdbeOneByteSerialTypeLen(u8);
#ifdef SQLITE_MIXED_ENDIAN_64BIT_FLOAT
  u64 sqlite3FloatSwap(u64 in);
# define swapMixedEndianFloat(X)  X = sqlite3FloatSwap(X)
#else
# define swapMixedEndianFloat(X)
#endif
void sqlite3VdbeSerialGet(const unsigned char*, tSerial, Mem*);
void sqlite3VdbeDeleteAuxData(sqlite3*, AuxData**, int, int);

int sqlite2BtreeKeyCompare(BtCursor *, const void *, int, int, int *);
