// AUTO-DRAFT from sqlite/sqlite PR #2f0c895a4e4098c6541cfb82a6a4dafb2abc1246
    }
    case 5: { /* nData==0 */
      aData = "";
      xDel = SQLITE_STATIC;  // <<< BUG ANCHOR
      isTransient = 0;
      isStatic = 0;
      break;
