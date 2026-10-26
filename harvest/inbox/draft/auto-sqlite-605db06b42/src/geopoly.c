// AUTO-DRAFT from sqlite/sqlite PR #50586b61d88cae30c7f3c247251b5ac0b360dc06
typedef float GeoCoord;
/* …（同文件无关代码省略）… */
#define GeoX(P,I)  (((GeoCoord*)(P)->a)[(I)*2])
#define GeoY(P,I)  (((GeoCoord*)(P)->a)[(I)*2+1])
/* …（同文件无关代码省略）… */
      sqlite3_str_appendf(x, "[%!g,%!g],", GeoX(p,i), GeoY(p,i));
    }
    sqlite3_str_appendf(x, "[%!g,%!g]]", GeoX(p,0), GeoY(p,0));
    sqlite3_result_text(context, sqlite3_str_finish(x), -1, sqlite3_free);  // <<< BUG ANCHOR
    sqlite3_free(p);
  }
}
/* …（同文件无关代码省略）… */
      }
    }
    sqlite3_str_appendf(x, "></polyline>");
    sqlite3_result_text(context, sqlite3_str_finish(x), -1, sqlite3_free);
    sqlite3_free(p);
  }
}
