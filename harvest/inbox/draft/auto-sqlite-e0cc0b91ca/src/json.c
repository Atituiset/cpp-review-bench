// AUTO-DRAFT from sqlite/sqlite PR #1a434126ab2bd95fab2c65c0d6d458d2674b5540
        }
      }else{
        if( !sqlite3Isdigit(z[i+1]) ){
          /* JSON5 allows for "+Infinity" and "-Infinity" using exactly
          ** that case.  SQLite also allows these in any case and it allows
          ** "+inf" and "-inf". */
/* …（同文件无关代码省略）… */
            t |= 0x01;
            goto parse_number_2;
          }
          pParse->iErr = i;
          return -1;
        }
