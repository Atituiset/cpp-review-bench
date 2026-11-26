// AUTO-DRAFT from sqlite/sqlite PR #915d9c4acea4a9036d1df7dcf9e8bd747ba726c4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct Qrf Qrf;
/* …（同文件无关代码省略）… */
static void qrfError(
  Qrf *p,                /* Query result state */
  int iCode,             /* Error code */
  const char *zFormat,   /* Message format (or NULL) */
  ...
){
  p->iErr = iCode;
  if( p->pzErr!=0 ){
    sqlite3_free(*p->pzErr);
    *p->pzErr = 0;
    if( zFormat ){
      va_list ap;
      va_start(ap, zFormat);
      *p->pzErr = sqlite3_vmprintf(zFormat, ap);
      va_end(ap);
    }
  }
}
/* …（同文件无关代码省略）… */
static void qrfWrite(Qrf *p){
  int n;
  if( p->spec.xWrite && (n = sqlite3_str_length(p->pOut))>0 ){
    int rc = p->spec.xWrite(p->spec.pWriteArg,
                 sqlite3_str_value(p->pOut),
                 (sqlite3_int64)n);
    sqlite3_str_reset(p->pOut);
    if( rc ){
      qrfError(p, rc, "Failed to write %d bytes of output", n);
    }
  }
}
/* …（同文件无关代码省略）… */
static void qrfEscape(
  int eEsc,            /* QRF_ESC_Ascii or QRF_ESC_Symbol */
  sqlite3_str *pStr,      /* String to be escaped */
  int iStart              /* Begin escapding on this byte of pStr */
){
  sqlite3_int64 i, j;     /* Loop counters */
  sqlite3_int64 nCtrl = 0;/* Number of control characters to escape */
  unsigned char *zIn;     /* Text to be escaped */
  unsigned int nIn;       /* Bytes of text to be escaped */
  unsigned char c;        /* A single character of the text */
  unsigned char *zOut;    /* Where to write the results */

  /* Find the text to be escaped */
  zIn = (unsigned char*)sqlite3_str_value(pStr);
  nIn = sqlite3_str_length(pStr);
  if( zIn==0 ) return;
  zIn += iStart;
  nIn -= iStart;

  /* Count the control characters */
  for(i=0; i<nIn; i++){
    if( (c = zIn[i])<=0x1f
     && c!='\t'
     && c!='\n'
     && (c!='\r' || zIn[i+1]!='\n')
    ){
      nCtrl++;
    }
  }
  if( nCtrl==0 ) return;  /* Early out if no control characters */

  /* Make space to hold the escapes.  Copy the original text to the end
  ** of the available space. */
  if( eEsc==QRF_ESC_Symbol ) nCtrl *= 2;
  sqlite3_str_appendchar(pStr, nCtrl, ' ');
  zOut = (unsigned char*)sqlite3_str_value(pStr);
  if( zOut==0 ) return;
  zOut += iStart;
  zIn = zOut + nCtrl;
  memmove(zIn,zOut,nIn);

  /* Convert the control characters */
  for(i=j=0; i<nIn; i++){
    if( (c = zIn[i])>0x1f
     || c=='\t'
     || c=='\n'
     || (c=='\r' && zIn[i+1]=='\n')
    ){
      continue;
    }
    if( i>0 ){
      memmove(&zOut[j], zIn, i);
      j += i;
    }
    zIn += i+1;
    nIn -= i+1;
    i = -1;
    if( eEsc==QRF_ESC_Symbol ){
      zOut[j++] = 0xe2;
      zOut[j++] = 0x90;
      zOut[j++] = 0x80+c;
    }else{
      zOut[j++] = '^';
      zOut[j++] = 0x40+c;
    }
  }
}
/* …（同文件无关代码省略）… */
          break;
        }
        default: {
          const void *pBlob = sqlite3_column_blob(p->pStmt,iCol);
          int nBlob = sqlite3_column_bytes(p->pStmt,iCol);
          int rc;
          qrfWrite(p);
          if( nBlob==0 ){
            /* no-op */
          }else if( p->spec.eEsc==QRF_ESC_Off ){
            rc = p->spec.xWrite(p->spec.pWriteArg,pBlob,nBlob);
            if( rc ){
              qrfError(p, rc, "Failed to write %d bytes of BLOB output", nBlob);
            }
          }else{
            sqlite3_str_append(pOut, pBlob, nBlob);
            qrfEscape(p->spec.eEsc, pOut, 0);
            qrfWrite(p);
          }
        }
      }
      break;
/* …（同文件无关代码省略）… */
      case QRF_TEXT_Csv:  p->spec.eBlob = QRF_BLOB_Tcl;  break;
      case QRF_TEXT_Tcl:  p->spec.eBlob = QRF_BLOB_Tcl;  break;
      case QRF_TEXT_Json: p->spec.eBlob = QRF_BLOB_Json; break;
      default:            p->spec.eBlob = QRF_BLOB_Text; break;
    }
  }
