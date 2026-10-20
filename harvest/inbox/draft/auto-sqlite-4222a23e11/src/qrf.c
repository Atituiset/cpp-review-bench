// AUTO-DRAFT from sqlite/sqlite PR #1f3e5e763dbee918ae8f8c44b84bb98f8d0fa2d6
    }
  }
  if( p->spec.eEsc==QRF_Auto ){
    p->spec.eEsc = QRF_ESC_Ascii;  // <<< BUG ANCHOR
  }
  if( p->spec.eText==QRF_Auto ){
    p->spec.eText = QRF_TEXT_Plain;
