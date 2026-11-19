// AUTO-DRAFT from sqlite/sqlite PR #27f8ba89beb1140e740817da85e1c6edc1b10093
  sqlite3VdbeAddOp4(v, OP_String8, 0, regString, 0, "", P4_STATIC);
  addrGe = sqlite3VdbeAddOp3(v, OP_Ge, regString, 0, regArith);
  VdbeCoverage(v);
  if( (op==OP_Ge && !bDesc) || (op==OP_Le && bDesc) ){  // <<< BUG ANCHOR
    sqlite3VdbeAddOp3(v, op, reg2, lbl, reg1); VdbeCoverage(v);
  }
  sqlite3VdbeAddOp3(v, OP_Add, regVal, regArith, regArith);
