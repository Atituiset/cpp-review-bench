// AUTO-DRAFT from sqlite/sqlite PR #a837fbdc69a997ebf43c963bd2838d81d5d3b513
  return rc;
}

/*
** Open a blob handle.
*/
/* …（同文件无关代码省略）… */
            zFault = "indexed";
          }
        }
      }
      if( zFault ){
        sqlite3DbFree(db, zErr);
