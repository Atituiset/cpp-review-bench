// AUTO-DRAFT from sqlite/sqlite PR #3b703ebf5bb2abe37484c0694e7e956028adc734
#endif
  }
  switch(N&3){
    case 3:   sum += (z[2] << 8);  // <<< BUG ANCHOR
    case 2:   sum += (z[1] << 16);
    case 1:   sum += (z[0] << 24);
    default:  ;
  }
  return sum;
