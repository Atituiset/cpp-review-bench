// AUTO-DRAFT from sqlite/sqlite PR #52a8a406eb9cda436b10842734c63050da6d8f13
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
static int isConsonant(const char *z){
  int j;
  char x = *z;
  if( x==0 ) return 0;
  assert( x>='a' && x<='z' );
  j = cType[x-'a'];
  if( j<2 ) return j;
  return z[1]==0 || isVowel(z + 1);
}
static int isVowel(const char *z){
  int j;
  char x = *z;
  if( x==0 ) return 0;
  assert( x>='a' && x<='z' );
  j = cType[x-'a'];
  if( j<2 ) return 1-j;
  return isConsonant(z + 1);
}
/* …（同文件无关代码省略）… */
static int m_gt_0(const char *z){
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  return *z!=0;
}
/* …（同文件无关代码省略）… */
static int m_eq_1(const char *z){
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 1;
  while( isConsonant(z) ){ z++; }
  return *z==0;
}
/* …（同文件无关代码省略）… */
static int m_gt_1(const char *z){
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  return *z!=0;
}
/* …（同文件无关代码省略）… */
static int hasVowel(const char *z){
  while( isConsonant(z) ){ z++; }
  return *z!=0;
}
/* …（同文件无关代码省略）… */
static int doubleConsonant(const char *z){
  return isConsonant(z) && z[0]==z[1];
}
/* …（同文件无关代码省略）… */
static int star_oh(const char *z){
  return
    isConsonant(z) &&
    z[0]!='w' && z[0]!='x' && z[0]!='y' &&
    isVowel(z+1) &&
    isConsonant(z+2);
}
/* …（同文件无关代码省略）… */
static int stem(
  char **pz,             /* The word being stemmed (Reversed) */
  const char *zFrom,     /* If the ending matches this... (Reversed) */
  const char *zTo,       /* ... change the ending to this (not reversed) */
  int (*xCond)(const char*)   /* Condition that must be true */
){
  char *z = *pz;
  while( *zFrom && *zFrom==*z ){ z++; zFrom++; }
  if( *zFrom!=0 ) return 0;
  if( xCond && !xCond(z) ) return 1;
  while( *zTo ){
    *(--z) = *(zTo++);
  }
  *pz = z;
  return 1;
}
/* …（同文件无关代码省略）… */
static void copy_stemmer(const char *zIn, int nIn, char *zOut, int *pnOut){
  int i, mx, j;
  int hasDigit = 0;
  for(i=0; i<nIn; i++){
    char c = zIn[i];
    if( c>='A' && c<='Z' ){
      zOut[i] = c - 'A' + 'a';
    }else{
      if( c>='0' && c<='9' ) hasDigit = 1;
      zOut[i] = c;
    }
  }
  mx = hasDigit ? 3 : 10;
  if( nIn>mx*2 ){
    for(j=mx, i=nIn-mx; i<nIn; i++, j++){
      zOut[j] = zOut[i];
    }
    i = j;
  }
  zOut[i] = 0;
  *pnOut = i;
}
/* …（同文件无关代码省略）… */
static void porter_stemmer(const char *zIn, int nIn, char *zOut, int *pnOut){
  int i, j;
  char zReverse[28];
  char *z, *z2;
  if( nIn<3 || nIn>=(int)sizeof(zReverse)-7 ){
    /* The word is too big or too small for the porter stemmer.
    ** Fallback to the copy stemmer */
    copy_stemmer(zIn, nIn, zOut, pnOut);
    return;
  }
  for(i=0, j=sizeof(zReverse)-6; i<nIn; i++, j--){
    char c = zIn[i];
    if( c>='A' && c<='Z' ){
      zReverse[j] = c + 'a' - 'A';
    }else if( c>='a' && c<='z' ){
      zReverse[j] = c;
    }else{
      /* The use of a character not in [a-zA-Z] means that we fallback
      ** to the copy stemmer */
      copy_stemmer(zIn, nIn, zOut, pnOut);
      return;
    }
  }
  memset(&zReverse[sizeof(zReverse)-5], 0, 5);
  z = &zReverse[j+1];


  /* Step 1a */
  if( z[0]=='s' ){
    if(
     !stem(&z, "sess", "ss", 0) &&
     !stem(&z, "sei", "i", 0)  &&
     !stem(&z, "ss", "ss", 0)
    ){
      z++;
    }
  }

  /* Step 1b */  
  z2 = z;
  if( stem(&z, "dee", "ee", m_gt_0) ){
    /* Do nothing.  The work was all in the test */
  }else if( 
     (stem(&z, "gni", "", hasVowel) || stem(&z, "de", "", hasVowel))
      && z!=z2
  ){
     if( stem(&z, "ta", "ate", 0) ||
         stem(&z, "lb", "ble", 0) ||
         stem(&z, "zi", "ize", 0) ){
       /* Do nothing.  The work was all in the test */
     }else if( doubleConsonant(z) && (*z!='l' && *z!='s' && *z!='z') ){
       z++;
     }else if( m_eq_1(z) && star_oh(z) ){
       *(--z) = 'e';
     }
  }

  /* Step 1c */
  if( z[0]=='y' && hasVowel(z+1) ){
    z[0] = 'i';
  }

  /* Step 2 */
  switch( z[1] ){
   case 'a':
     if( !stem(&z, "lanoita", "ate", m_gt_0) ){
       stem(&z, "lanoit", "tion", m_gt_0);
     }
     break;
   case 'c':
     if( !stem(&z, "icne", "ence", m_gt_0) ){
       stem(&z, "icna", "ance", m_gt_0);
     }
     break;
   case 'e':
     stem(&z, "rezi", "ize", m_gt_0);
     break;
   case 'g':
     stem(&z, "igol", "log", m_gt_0);
     break;
   case 'l':
     if( !stem(&z, "ilb", "ble", m_gt_0) 
      && !stem(&z, "illa", "al", m_gt_0)
      && !stem(&z, "iltne", "ent", m_gt_0)
      && !stem(&z, "ile", "e", m_gt_0)
     ){
       stem(&z, "ilsuo", "ous", m_gt_0);
     }
     break;
   case 'o':
     if( !stem(&z, "noitazi", "ize", m_gt_0)
      && !stem(&z, "noita", "ate", m_gt_0)
     ){
       stem(&z, "rota", "ate", m_gt_0);
     }
     break;
   case 's':
     if( !stem(&z, "msila", "al", m_gt_0)
      && !stem(&z, "ssenevi", "ive", m_gt_0)
      && !stem(&z, "ssenluf", "ful", m_gt_0)
     ){
       stem(&z, "ssensuo", "ous", m_gt_0);
     }
     break;
   case 't':
     if( !stem(&z, "itila", "al", m_gt_0)
      && !stem(&z, "itivi", "ive", m_gt_0)
     ){
       stem(&z, "itilib", "ble", m_gt_0);
     }
     break;
  }

  /* Step 3 */
  switch( z[0] ){
   case 'e':
     if( !stem(&z, "etaci", "ic", m_gt_0)
      && !stem(&z, "evita", "", m_gt_0)
     ){
       stem(&z, "ezila", "al", m_gt_0);
     }
     break;
   case 'i':
     stem(&z, "itici", "ic", m_gt_0);
     break;
   case 'l':
     if( !stem(&z, "laci", "ic", m_gt_0) ){
       stem(&z, "luf", "", m_gt_0);
     }
     break;
   case 's':
     stem(&z, "ssen", "", m_gt_0);
     break;
  }

  /* Step 4 */
  switch( z[1] ){
   case 'a':
     if( z[0]=='l' && m_gt_1(z+2) ){
       z += 2;
     }
     break;
   case 'c':
     if( z[0]=='e' && z[2]=='n' && (z[3]=='a' || z[3]=='e')  && m_gt_1(z+4)  ){
       z += 4;
     }
     break;
   case 'e':
     if( z[0]=='r' && m_gt_1(z+2) ){
       z += 2;
     }
     break;
   case 'i':
     if( z[0]=='c' && m_gt_1(z+2) ){
       z += 2;
     }
     break;
   case 'l':
     if( z[0]=='e' && z[2]=='b' && (z[3]=='a' || z[3]=='i') && m_gt_1(z+4) ){
       z += 4;
     }
     break;
   case 'n':
     if( z[0]=='t' ){
       if( z[2]=='a' ){
         if( m_gt_1(z+3) ){
           z += 3;
         }
       }else if( z[2]=='e' ){
         if( !stem(&z, "tneme", "", m_gt_1)
          && !stem(&z, "tnem", "", m_gt_1)
         ){
           stem(&z, "tne", "", m_gt_1);
         }
       }
     }
     break;
   case 'o':
     if( z[0]=='u' ){
       if( m_gt_1(z+2) ){
         z += 2;
       }
     }else if( z[3]=='s' || z[3]=='t' ){
       stem(&z, "noi", "", m_gt_1);
     }
     break;
   case 's':
     if( z[0]=='m' && z[2]=='i' && m_gt_1(z+3) ){
       z += 3;
     }
     break;
   case 't':
     if( !stem(&z, "eta", "", m_gt_1) ){
       stem(&z, "iti", "", m_gt_1);
     }
     break;
   case 'u':
     if( z[0]=='s' && z[2]=='o' && m_gt_1(z+3) ){
       z += 3;
     }
     break;
   case 'v':
   case 'z':
     if( z[0]=='e' && z[2]=='i' && m_gt_1(z+3) ){
       z += 3;
     }
     break;
  }

  /* Step 5a */
  if( z[0]=='e' ){
    if( m_gt_1(z+1) ){
      z++;
    }else if( m_eq_1(z+1) && !star_oh(z+1) ){
      z++;
    }
  }

  /* Step 5b */
  if( m_gt_1(z) && z[0]=='l' && z[1]=='l' ){
    z++;
  }

  /* z[] is now the stemmed word in reverse order.  Flip it back
  ** around into forward order and return.
  */
  *pnOut = i = (int)strlen(z);
  zOut[i] = 0;
  while( *z ){
    zOut[--i] = *(z++);
  }
}
/* …（同文件无关代码省略）… */
    if( c->iOffset>iStartOffset ){
      int n = c->iOffset-iStartOffset;
      if( n>c->nAllocated ){
        char *pNew;
        c->nAllocated = n+20;
        pNew = sqlite3_realloc64(c->zToken, c->nAllocated);
        if( !pNew ) return SQLITE_NOMEM;
        c->zToken = pNew;
      }
      porter_stemmer(&z[iStartOffset], n, c->zToken, pnBytes);
