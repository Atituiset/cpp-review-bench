// AUTO-DRAFT from sqlite/sqlite PR #9e7e99e39356aabd9148a102a11bf1573691d2c1
  i64 nByte;             /* Data space required for this record */
  i64 nZero;             /* Number of zero bytes at the end of the record */
  int nVarint;           /* Number of bytes in a varint */
  u32 serial_type;       /* Type field */  // <<< BUG ANCHOR
  Mem *pData0;           /* First field to be combined into the record */
  Mem *pLast;            /* Last field of the record */
  int nField;            /* Number of fields in the record */
/* …（同文件无关代码省略）… */
      }
      nData += len;
      nHdr += sqlite3VarintLen(serial_type);
      pRec->uTemp = serial_type;
    }
    if( pRec==pData0 ) break;
    pRec--;
