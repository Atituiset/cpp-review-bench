// AUTO-DRAFT from sqlite/sqlite PR #77f8caa273d6d30f3f0e278b2ac04715a87b37df
  i64 nByte;             /* Data space required for this record */
  i64 nZero;             /* Number of zero bytes at the end of the record */
  int nVarint;           /* Number of bytes in a varint */
  tSerial serial_type;   /* Type field */  // <<< BUG ANCHOR
  Mem *pData0;           /* First field to be combined into the record */
  Mem *pLast;            /* Last field of the record */
  int nField;            /* Number of fields in the record */
/* …（同文件无关代码省略）… */
        }else{
          nZero += pRec->u.nZero;
        }
        serial_type += (u32)pRec->u.nZero*2;
      }
      nData += len;
      nHdr += sqlite3VarintLen(serial_type);
