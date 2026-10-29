// AUTO-DRAFT from sqlite/sqlite PR #3cf93639e3685fb66149ce5f407f036fb2ea395e
      case SQLITE_INTEGER:
      case SQLITE_FLOAT:
        b = log(x);
        if( b<=0.0 ) return;  // <<< BUG ANCHOR
        x = sqlite3_value_double(argv[1]);
        if( x<=0.0 ) return;
        break;
