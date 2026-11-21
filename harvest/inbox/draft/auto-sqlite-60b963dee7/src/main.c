// AUTO-DRAFT from sqlite/sqlite PR #7dd00550bf588547c66f1084f531d91b28805910
      break;
    }
    case SQLITE_DBCONFIG_FP_DIGITS: {
      int nIn = va_arg(ap, int);
      int *pOut = va_arg(ap, int*);
      if( nIn>3 && nIn<24 ) db->nFpDigit = (u8)nIn;  // <<< BUG ANCHOR
      if( pOut ) *pOut = db->nFpDigit;
      rc = SQLITE_OK;
      break;
    }
    default: {
/* …（同文件无关代码省略）… */
#endif
      break;
    }
  }
  va_end(ap);
#endif /* SQLITE_UNTESTABLE */
