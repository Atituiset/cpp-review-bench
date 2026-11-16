// AUTO-DRAFT from sqlite/sqlite PR #8a24548dfbe71bc2714079e7549d1814002f5c44
      break;
    }
  // <<< BUG ANCHOR
#if defined(SQLITE_DEBUG) && !defined(SQLITE_OMIT_WSD)
    /* sqlite3_test_control(SQLITE_TESTCTRL_TUNE, id, *piValue)
    **
    ** If "id" is an integer between 1 and SQLITE_NTUNE then set the value
    ** of the id-th tuning parameter to *piValue.  If "id" is between -1
    ** and -SQLITE_NTUNE, then write the current value of the (-id)-th
    ** tuning parameter into *piValue.
    **
    ** Tuning parameters are for use during transient development builds,
    ** to help find the best values for constants in the query planner.
    ** Access tuning parameters using the Tuning(ID) macro.  Set the
    ** parameters in the CLI using ".testctrl tune ID VALUE".
    **
    ** Transient use only.  Tuning parameters should not be used in
    ** checked-in code.
    */
    case SQLITE_TESTCTRL_TUNE: {
      int id = va_arg(ap, int);
      int *piValue = va_arg(ap, int*);
      if( id>0 && id<=SQLITE_NTUNE ){
        Tuning(id) = *piValue;
      }else if( id<0 && id>=-SQLITE_NTUNE ){
        *piValue = Tuning(-id);
      }else{
        rc = SQLITE_NOTFOUND;
      }
      break;
    }
