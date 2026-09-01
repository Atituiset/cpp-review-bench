// AUTO-DRAFT from sqlite/sqlite PR #638e0b1a03f88ed4b08b8e534c374e6f59c1e2f7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define ISUPPER(X) isupper((unsigned char)(X))
/* …（同文件无关代码省略）… */
#define PRIVATE
/* …（同文件无关代码省略）… */
#define lemonStrlen(X)   ((int)strlen(X))
/* …（同文件无关代码省略）… */
typedef struct MemChunk MemChunk;
/* …（同文件无关代码省略）… */
static void *lemon_malloc(size_t nByte){
  MemChunk *p;
  if( nByte<0 ) return 0;
  p = malloc( nByte + sizeof(MemChunk) );
  if( p==0 ){
    fprintf(stderr, "Out of memory.  Failed to allocate %lld bytes.\n",
            (long long int)nByte);
    exit(1);
  }
  p->pNext = memChunkList;
  p->sz = nByte;
  memChunkList = p;
  return (void*)&p[1];
}
static void *lemon_calloc(size_t nElem, size_t sz){
  void *p = lemon_malloc(nElem*sz);
  memset(p, 0, nElem*sz);
  return p;
}
/* …（同文件无关代码省略）… */
static void *lemon_realloc(void *pOld, size_t nNew){
  void *pNew;
  MemChunk *p;
  if( pOld==0 ) return lemon_malloc(nNew);
  p = (MemChunk*)pOld;
  p--;
  if( p->sz>=nNew ) return pOld;
  pNew = lemon_malloc( nNew );
  memcpy(pNew, pOld, p->sz);
  return pNew;
}
/* …（同文件无关代码省略）… */
static void lemon_strcpy(char *dest, const char *src){
  while( (*(dest++) = *(src++))!=0 ){}
}
/* …（同文件无关代码省略）… */
enum option_type { OPT_FLAG=1,  OPT_INT,  OPT_DBL,  OPT_STR,
         OPT_FFLAG, OPT_FINT, OPT_FDBL, OPT_FSTR};
struct s_options {
  enum option_type type;
  const char *label;
  char *arg;
  const char *message;
};
/* …（同文件无关代码省略）… */
#define SetFind(X,Y) (X[Y])       /* True if Y is in set X */
/* …（同文件无关代码省略）… */
typedef enum {LEMON_FALSE=0, LEMON_TRUE} Boolean;
/* …（同文件无关代码省略）… */
enum symbol_type {
  TERMINAL,
  NONTERMINAL,
  MULTITERMINAL
};
enum e_assoc {
    LEFT,
    RIGHT,
    NONE,
    UNK
};
struct symbol {
  const char *name;        /* Name of the symbol */
  int index;               /* Index number for this symbol */
  enum symbol_type type;   /* Symbols are all either TERMINALS or NTs */
  struct rule *rule;       /* Linked list of rules of this (if an NT) */
  struct symbol *fallback; /* fallback token in case this token doesn't parse */
  int prec;                /* Precedence if defined (-1 otherwise) */
  enum e_assoc assoc;      /* Associativity if precedence is defined */
  char *firstset;          /* First-set for all rules of this symbol */
  Boolean lambda;          /* True if NT and can generate an empty string */
  int useCnt;              /* Number of times used */
  char *destructor;        /* Code which executes whenever this symbol is
                           ** popped from the stack during error processing */
  int destLineno;          /* Line number for start of destructor.  Set to
                           ** -1 for duplicate destructors. */
  char *datatype;          /* The data type of information held by this
                           ** object. Only used if type==NONTERMINAL */
  int dtnum;               /* The data type number.  In the parser, the value
                           ** stack is a union.  The .yy%d element of this
                           ** union is the correct data type for this object */
  int bContent;            /* True if this symbol ever carries content - if
                           ** it is ever more than just syntax */
  /* The following fields are used by MULTITERMINALs only */
  int nsubsym;             /* Number of constituent symbols in the MULTI */
  struct symbol **subsym;  /* Array of constituent symbols */
};
/* …（同文件无关代码省略）… */
struct rule {
  struct symbol *lhs;      /* Left-hand side of the rule */
  const char *lhsalias;    /* Alias for the LHS (NULL if none) */
  int lhsStart;            /* True if left-hand side is the start symbol */
  int ruleline;            /* Line number for the rule */
  int nrhs;                /* Number of RHS symbols */
  struct symbol **rhs;     /* The RHS symbols */
  const char **rhsalias;   /* An alias for each RHS symbol (NULL if none) */
  int line;                /* Line number at which code begins */
  const char *code;        /* The code executed when this rule is reduced */
  const char *codePrefix;  /* Setup code before code[] above */
  const char *codeSuffix;  /* Breakdown code after code[] above */
  struct symbol *precsym;  /* Precedence symbol for this rule */
  int index;               /* An index number for this rule */
  int iRule;               /* Rule number as used in the generated tables */
  Boolean noCode;          /* True if this rule has no associated C code */
  Boolean codeEmitted;     /* True if the code has been emitted already */
  Boolean canReduce;       /* True if this rule is ever reduced */
  Boolean doesReduce;      /* Reduce actions occur after optimization */
  Boolean neverReduce;     /* Reduce is theoretically possible, but prevented
                           ** by actions or other outside implementation */
  struct rule *nextlhs;    /* Next rule with the same LHS */
  struct rule *next;       /* Next rule in the global list */
};
/* …（同文件无关代码省略）… */
enum cfgstatus {
  COMPLETE,
  INCOMPLETE
};
struct config {
  struct rule *rp;         /* The rule upon which the configuration is based */
  int dot;                 /* The parse point */
  char *fws;               /* Follow-set for this configuration only */
  struct plink *fplp;      /* Follow-set forward propagation links */
  struct plink *bplp;      /* Follow-set backwards propagation links */
  struct state *stp;       /* Pointer to state which contains this */
  enum cfgstatus status;   /* used during followset and shift computations */
  struct config *next;     /* Next configuration in the state */
  struct config *bp;       /* The next basis configuration */
};
/* …（同文件无关代码省略）… */
enum e_action {
  SHIFT,
  ACCEPT,
  REDUCE,
  ERROR,
  SSCONFLICT,              /* A shift/shift conflict */
  SRCONFLICT,              /* Was a reduce, but part of a conflict */
  RRCONFLICT,              /* Was a reduce, but part of a conflict */
  SH_RESOLVED,             /* Was a shift.  Precedence resolved conflict */
  RD_RESOLVED,             /* Was reduce.  Precedence resolved conflict */
  NOT_USED,                /* Deleted by compression */
  SHIFTREDUCE              /* Shift first, then reduce */
};
/* …（同文件无关代码省略）… */
struct action {
  struct symbol *sp;       /* The look-ahead symbol */
  enum e_action type;
  union {
    struct state *stp;     /* The new state, if a shift */
    struct rule *rp;       /* The rule, if a reduce */
  } x;
  struct symbol *spOpt;    /* SHIFTREDUCE optimization to this symbol */
  struct action *next;     /* Next action for this state */
  struct action *collide;  /* Next action with the same hash */
};
/* …（同文件无关代码省略）… */
struct state {
  struct config *bp;       /* The basis configurations for this state */
  struct config *cfp;      /* All configurations in this set */
  int statenum;            /* Sequential number for this state */
  struct action *ap;       /* List of actions for this state */
  int nTknAct, nNtAct;     /* Number of actions on terminals and nonterminals */
  int iTknOfst, iNtOfst;   /* yy_action[] offset for terminals and nonterms */
  int iDfltReduce;         /* Default action is to REDUCE by this rule */
  struct rule *pDfltReduce;/* The default REDUCE rule. */
  int autoReduce;          /* True if this is an auto-reduce state */
};
/* …（同文件无关代码省略）… */
struct plink {
  struct config *cfp;      /* The configuration to which linked */
  struct plink *next;      /* The next propagate link */
};
/* …（同文件无关代码省略）… */
struct lemon {
  struct state **sorted;   /* Table of states sorted by state number */
  struct rule *rule;       /* List of all rules */
  struct rule *startRule;  /* First rule */
  int nstate;              /* Number of states */
  int nxstate;             /* nstate with tail degenerate states removed */
  int nrule;               /* Number of rules */
  int nruleWithAction;     /* Number of rules with actions */
  int nsymbol;             /* Number of terminal and nonterminal symbols */
  int nterminal;           /* Number of terminal symbols */
  int minShiftReduce;      /* Minimum shift-reduce action value */
  int errAction;           /* Error action value */
  int accAction;           /* Accept action value */
  int noAction;            /* No-op action value */
  int minReduce;           /* Minimum reduce action */
  int maxAction;           /* Maximum action value of any kind */
  struct symbol **symbols; /* Sorted array of pointers to symbols */
  int errorcnt;            /* Number of errors */
  struct symbol *errsym;   /* The error symbol */
  struct symbol *wildcard; /* Token that matches anything */
  char *name;              /* Name of the generated parser */
  char *arg;               /* Declaration of the 3rd argument to parser */
  char *ctx;               /* Declaration of 2nd argument to constructor */
  char *tokentype;         /* Type of terminal symbols in the parser stack */
  char *vartype;           /* The default type of non-terminal symbols */
  char *start;             /* Name of the start symbol for the grammar */
  char *stacksize;         /* Size of the parser stack */
  char *include;           /* Code to put at the start of the C file */
  char *error;             /* Code to execute when an error is seen */
  char *overflow;          /* Code to execute on a stack overflow */
  char *failure;           /* Code to execute on parser failure */
  char *accept;            /* Code to execute when the parser excepts */
  char *extracode;         /* Code appended to the generated file */
  char *tokendest;         /* Code to execute to destroy token data */
  char *vardest;           /* Code for the default non-terminal destructor */
  char *filename;          /* Name of the input file */
  char *outname;           /* Name of the current output file */
  char *tokenprefix;       /* A prefix added to token names in the .h file */
  char *stackSizeLimit;    /* Function to return the stack size limit */
  char *reallocFunc;       /* Function to use to allocate stack space */
  char *freeFunc;          /* Function to use to free stack space */
  int nconflict;           /* Number of parsing conflicts */
  int nactiontab;          /* Number of entries in the yy_action[] table */
  int nlookaheadtab;       /* Number of entries in yy_lookahead[] */
  int tablesize;           /* Total table size of all tables in bytes */
  int basisflag;           /* Print only basis configurations */
  int printPreprocessed;   /* Show preprocessor output on stdout */
  int has_fallback;        /* True if any %fallback is seen in the grammar */
  int nolinenosflag;       /* True if #line statements should not be printed */
  int argc;                /* Number of command-line arguments */
  char **argv;             /* Command-line arguments */
};
/* …（同文件无关代码省略）… */
#define MemoryCheck(X) if((X)==0){ \
  extern void memory_error(); \
  memory_error(); \
}
/* …（同文件无关代码省略）… */
static struct action *Action_new(void){
  static struct action *actionfreelist = 0;
  struct action *newaction;

  if( actionfreelist==0 ){
    int i;
    int amt = 100;
    actionfreelist = (struct action *)lemon_calloc(amt, sizeof(struct action));
    if( actionfreelist==0 ){
      fprintf(stderr,"Unable to allocate memory for a new parser action.");
      exit(1);
    }
    for(i=0; i<amt-1; i++) actionfreelist[i].next = &actionfreelist[i+1];
    actionfreelist[amt-1].next = 0;
  }
  newaction = actionfreelist;
  actionfreelist = actionfreelist->next;
  return newaction;
}
/* …（同文件无关代码省略）… */
static int actioncmp(
  struct action *ap1,
  struct action *ap2
){
  int rc;
  rc = ap1->sp->index - ap2->sp->index;
  if( rc==0 ){
    rc = (int)ap1->type - (int)ap2->type;
  }
  if( rc==0 && (ap1->type==REDUCE || ap1->type==SHIFTREDUCE) ){
    rc = ap1->x.rp->index - ap2->x.rp->index;
  }
  if( rc==0 ){
    rc = (int) (ap2 - ap1);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static struct action *Action_sort(
  struct action *ap
){
  ap = (struct action *)msort((char *)ap,(char **)&ap->next,
                              (int(*)(const char*,const char*))actioncmp);
  return ap;
}
/* …（同文件无关代码省略）… */
void Action_add(
  struct action **app,
  enum e_action type,
  struct symbol *sp,
  char *arg
){
  struct action *newaction;
  newaction = Action_new();
  newaction->next = *app;
  *app = newaction;
  newaction->type = type;
  newaction->sp = sp;
  newaction->spOpt = 0;
  if( type==SHIFT ){
    newaction->x.stp = (struct state *)arg;
  }else{
    newaction->x.rp = (struct rule *)arg;
  }
}
/* …（同文件无关代码省略）… */
void FindActions(struct lemon *lemp)
{
  int i,j;
  struct config *cfp;
  struct state *stp;
  struct symbol *sp;
  struct rule *rp;

  /* Add all of the reduce actions
  ** A reduce action is added for each element of the followset of
  ** a configuration which has its dot at the extreme right.
  */
  for(i=0; i<lemp->nstate; i++){   /* Loop over all states */
    stp = lemp->sorted[i];
    for(cfp=stp->cfp; cfp; cfp=cfp->next){  /* Loop over all configurations */
      if( cfp->rp->nrhs==cfp->dot ){        /* Is dot at extreme right? */
        for(j=0; j<lemp->nterminal; j++){
          if( SetFind(cfp->fws,j) ){
            /* Add a reduce action to the state "stp" which will reduce by the
            ** rule "cfp->rp" if the lookahead symbol is "lemp->symbols[j]" */
            Action_add(&stp->ap,REDUCE,lemp->symbols[j],(char *)cfp->rp);
          }
        }
      }
    }
  }

  /* Add the accepting token */
  if( lemp->start ){
    sp = Symbol_find(lemp->start);
    if( sp==0 ){
      if( lemp->startRule==0 ){
        fprintf(stderr, "internal error on source line %d: no start rule\n",
                __LINE__);
        exit(1);
      }
      sp = lemp->startRule->lhs;
    }
  }else{
    sp = lemp->startRule->lhs;
  }
  /* Add to the first state (which is always the starting state of the
  ** finite state machine) an action to ACCEPT if the lookahead is the
  ** start nonterminal.  */
  Action_add(&lemp->sorted[0]->ap,ACCEPT,sp,0);

  /* Resolve conflicts */
  for(i=0; i<lemp->nstate; i++){
    struct action *ap, *nap;
    stp = lemp->sorted[i];
    /* assert( stp->ap ); */
    stp->ap = Action_sort(stp->ap);
    for(ap=stp->ap; ap && ap->next; ap=ap->next){
      for(nap=ap->next; nap && nap->sp==ap->sp; nap=nap->next){
         /* The two actions "ap" and "nap" have the same lookahead.
         ** Figure out which one should be used */
         lemp->nconflict += resolve_conflict(ap,nap);
      }
    }
  }

  /* Report an error for each rule that can never be reduced. */
  for(rp=lemp->rule; rp; rp=rp->next) rp->canReduce = LEMON_FALSE;
  for(i=0; i<lemp->nstate; i++){
    struct action *ap;
    for(ap=lemp->sorted[i]->ap; ap; ap=ap->next){
      if( ap->type==REDUCE ) ap->x.rp->canReduce = LEMON_TRUE;
    }
  }
  for(rp=lemp->rule; rp; rp=rp->next){
    if( rp->canReduce ) continue;
    ErrorMsg(lemp->filename,rp->ruleline,"This rule can not be reduced.\n");
    lemp->errorcnt++;
  }
}
/* …（同文件无关代码省略）… */
static int resolve_conflict(
  struct action *apx,
  struct action *apy
){
  struct symbol *spx, *spy;
  int errcnt = 0;
  assert( apx->sp==apy->sp );  /* Otherwise there would be no conflict */
  if( apx->type==SHIFT && apy->type==SHIFT ){
    apy->type = SSCONFLICT;
    errcnt++;
  }
  if( apx->type==SHIFT && apy->type==REDUCE ){
    spx = apx->sp;
    spy = apy->x.rp->precsym;
    if( spy==0 || spx->prec<0 || spy->prec<0 ){
      /* Not enough precedence information. */
      apy->type = SRCONFLICT;
      errcnt++;
    }else if( spx->prec>spy->prec ){    /* higher precedence wins */
      apy->type = RD_RESOLVED;
    }else if( spx->prec<spy->prec ){
      apx->type = SH_RESOLVED;
    }else if( spx->prec==spy->prec && spx->assoc==RIGHT ){ /* Use operator */
      apy->type = RD_RESOLVED;                             /* associativity */
    }else if( spx->prec==spy->prec && spx->assoc==LEFT ){  /* to break tie */
      apx->type = SH_RESOLVED;
    }else{
      assert( spx->prec==spy->prec && spx->assoc==NONE );
      apx->type = ERROR;
    }
  }else if( apx->type==REDUCE && apy->type==REDUCE ){
    spx = apx->x.rp->precsym;
    spy = apy->x.rp->precsym;
    if( spx==0 || spy==0 || spx->prec<0 ||
    spy->prec<0 || spx->prec==spy->prec ){
      apy->type = RRCONFLICT;
      errcnt++;
    }else if( spx->prec>spy->prec ){
      apy->type = RD_RESOLVED;
    }else if( spx->prec<spy->prec ){
      apx->type = RD_RESOLVED;
    }
  }else{
    assert(
      apx->type==ERROR ||
      apx->type==SH_RESOLVED ||
      apx->type==RD_RESOLVED ||
      apx->type==SSCONFLICT ||
      apx->type==SRCONFLICT ||
      apx->type==RRCONFLICT ||
      apy->type==SH_RESOLVED ||
      apy->type==RD_RESOLVED ||
      apy->type==SSCONFLICT ||
      apy->type==SRCONFLICT ||
      apy->type==RRCONFLICT
    );
    /* The REDUCE/SHIFT case cannot happen because SHIFTs come before
    ** REDUCEs on the list.  If we reach this point it must be because
    ** the parser conflict had already been resolved. */
  }
  return errcnt;
}
/* …（同文件无关代码省略）… */
void ErrorMsg(const char *filename, int lineno, const char *format, ...){
  va_list ap;
  fprintf(stderr, "%s:%d: ", filename, lineno);
  va_start(ap, format);
  vfprintf(stderr,format,ap);
  va_end(ap);
  fprintf(stderr, "\n");
}
/* …（同文件无关代码省略）… */
void memory_error(void){
  fprintf(stderr,"Out of memory.  Aborting...\n");
  exit(1);
}
/* …（同文件无关代码省略）… */
static void handle_D_option(char *z){
  char **paz;
  nDefine++;
  azDefine = (char **) lemon_realloc(azDefine, sizeof(azDefine[0])*nDefine);
  if( azDefine==0 ){
    fprintf(stderr,"out of memory\n");
    exit(1);
  }
  bDefineUsed = (char*)lemon_realloc(bDefineUsed, nDefine);
  if( bDefineUsed==0 ){
    fprintf(stderr,"out of memory\n");
    exit(1);
  }
  bDefineUsed[nDefine-1] = 0;
  paz = &azDefine[nDefine-1];
  *paz = (char *) lemon_malloc( lemonStrlen(z)+1 );
  if( *paz==0 ){
    fprintf(stderr,"out of memory\n");
    exit(1);
  }
  lemon_strcpy(*paz, z);
  for(z=*paz; *z && *z!='='; z++){}
  *z = 0;
}
/* …（同文件无关代码省略）… */
static void handle_d_option(char *z){
  outputDir = (char *) lemon_malloc( lemonStrlen(z)+1 );
  if( outputDir==0 ){
    fprintf(stderr,"out of memory\n");
    exit(1);
  }
  lemon_strcpy(outputDir, z);
}
/* …（同文件无关代码省略）… */
  static int version = 0;
  static int rpflag = 0;
  static int basisflag = 0;
  static int compress = 0;
  static int quiet = 0;
  static int statistics = 0;
  static int mhflag = 0;
/* …（同文件无关代码省略）… */
  
  static struct s_options options[] = {
    {OPT_FLAG, "b", (char*)&basisflag, "Print only the basis in report."},
    {OPT_FLAG, "c", (char*)&compress, "Don't compress the action table."},
    {OPT_FSTR, "d", (char*)&handle_d_option, "Output directory.  Default '.'"},
    {OPT_FSTR, "D", (char*)handle_D_option, "Define an %ifdef macro."},
    {OPT_FLAG, "E", (char*)&printPP, "Print input file after preprocessing."},
/* …（同文件无关代码省略）… */
    FindActions(&lem);

    /* Compress the action tables */
    if( compress==0 ) CompressTables(&lem);

    /* Reorder and renumber the states so that states with fewer choices
    ** occur at the end.  This is an optimization that helps make the
/* …（同文件无关代码省略）… */
#define NEXT(A) (*(char**)(((char*)A)+offset))
/* …（同文件无关代码省略）… */
static char *merge(
  char *a,
  char *b,
  int (*cmp)(const char*,const char*),
  int offset
){
  char *ptr, *head;

  if( a==0 ){
    head = b;
  }else if( b==0 ){
    head = a;
  }else{
    if( (*cmp)(a,b)<=0 ){
      ptr = a;
      a = NEXT(a);
    }else{
      ptr = b;
      b = NEXT(b);
    }
    head = ptr;
    while( a && b ){
      if( (*cmp)(a,b)<=0 ){
        NEXT(ptr) = a;
        ptr = a;
        a = NEXT(a);
      }else{
        NEXT(ptr) = b;
        ptr = b;
        b = NEXT(b);
      }
    }
    if( a ) NEXT(ptr) = a;
    else    NEXT(ptr) = b;
  }
  return head;
}
/* …（同文件无关代码省略）… */
#define LISTSIZE 30
static char *msort(
  char *list,
  char **next,
  int (*cmp)(const char*,const char*)
){
  unsigned long offset;
  char *ep;
  char *set[LISTSIZE];
  int i;
  offset = (unsigned long)((char*)next - (char*)list);
  for(i=0; i<LISTSIZE; i++) set[i] = 0;
  while( list ){
    ep = list;
    list = NEXT(list);
    NEXT(ep) = 0;
    for(i=0; i<LISTSIZE-1 && set[i]!=0; i++){
      ep = merge(set[i],ep,cmp,offset);
      set[i] = 0;
    }
    set[i] = merge(set[i],ep,cmp,offset);
  }
  ep = 0;
  for(i=0; i<LISTSIZE; i++) if( set[i] ) ep = merge(set[i],ep,cmp,offset);
  return ep;
}
/* …（同文件无关代码省略）… */
void CompressTables(struct lemon *lemp)
{
  struct state *stp;
  struct action *ap, *ap2, *nextap;
  struct rule *rp, *rp2, *rbest;
  int nbest, n;
  int i;
  int usesWildcard;

  for(i=0; i<lemp->nstate; i++){
    stp = lemp->sorted[i];
    nbest = 0;
    rbest = 0;
    usesWildcard = 0;

    for(ap=stp->ap; ap; ap=ap->next){
      if( ap->type==SHIFT && ap->sp==lemp->wildcard ){
        usesWildcard = 1;
      }
      if( ap->type!=REDUCE ) continue;
      rp = ap->x.rp;
      if( rp->lhsStart ) continue;
      if( rp==rbest ) continue;
      n = 1;
      for(ap2=ap->next; ap2; ap2=ap2->next){
        if( ap2->type!=REDUCE ) continue;
        rp2 = ap2->x.rp;
        if( rp2==rbest ) continue;
        if( rp2==rp ) n++;
      }
      if( n>nbest ){
        nbest = n;
        rbest = rp;
      }
    }

    /* Do not make a default if the number of rules to default
    ** is not at least 1 or if the wildcard token is a possible
    ** lookahead.
    */
    if( nbest<1 || usesWildcard ) continue;


    /* Combine matching REDUCE actions into a single default */
    for(ap=stp->ap; ap; ap=ap->next){
      if( ap->type==REDUCE && ap->x.rp==rbest ) break;
    }
    assert( ap );
    ap->sp = Symbol_new("{default}");
    for(ap=ap->next; ap; ap=ap->next){
      if( ap->type==REDUCE && ap->x.rp==rbest ) ap->type = NOT_USED;
    }
    stp->ap = Action_sort(stp->ap);

    for(ap=stp->ap; ap; ap=ap->next){
      if( ap->type==SHIFT ) break;
      if( ap->type==REDUCE && ap->x.rp!=rbest ) break;
    }
    if( ap==0 ){
      stp->autoReduce = 1;
      stp->pDfltReduce = rbest;
    }
  }

  /* Make a second pass over all states and actions.  Convert
  ** every action that is a SHIFT to an autoReduce state into
  ** a SHIFTREDUCE action.
  */
  for(i=0; i<lemp->nstate; i++){
    stp = lemp->sorted[i];
    for(ap=stp->ap; ap; ap=ap->next){
      struct state *pNextState;
      if( ap->type!=SHIFT ) continue;
      pNextState = ap->x.stp;
      if( pNextState->autoReduce && pNextState->pDfltReduce!=0 ){
        ap->type = SHIFTREDUCE;
        ap->x.rp = pNextState->pDfltReduce;
      }
    }
  }

  /* If a SHIFTREDUCE action specifies a rule that has a single RHS term
  ** (meaning that the SHIFTREDUCE will land back in the state where it
  ** started) and if there is no C-code associated with the reduce action,
  ** then we can go ahead and convert the action to be the same as the
  ** action for the RHS of the rule.
  */
  for(i=0; i<lemp->nstate; i++){
    stp = lemp->sorted[i];
    for(ap=stp->ap; ap; ap=nextap){
      nextap = ap->next;
      if( ap->type!=SHIFTREDUCE ) continue;
      rp = ap->x.rp;
      if( rp->noCode==0 ) continue;
      if( rp->nrhs!=1 ) continue;
#if 1
      /* Only apply this optimization to non-terminals.  It would be OK to
      ** apply it to terminal symbols too, but that makes the parser tables
      ** larger. */
      if( ap->sp->index<lemp->nterminal ) continue;
#endif
      /* If we reach this point, it means the optimization can be applied */
      nextap = ap;
      for(ap2=stp->ap; ap2 && (ap2==ap || ap2->sp!=rp->lhs); ap2=ap2->next){}
      assert( ap2!=0 );
      ap->spOpt = ap2->sp;
      ap->type = ap2->type;
      ap->x = ap2->x;
    }
  }
}
/* …（同文件无关代码省略）… */
PRIVATE unsigned strhash(const char *x)
{
  unsigned h = 0;
  while( *x ) h = h*13 + *(x++);
  return h;
}
/* …（同文件无关代码省略）… */
const char *Strsafe(const char *y)
{
  const char *z;
  char *cpy;

  if( y==0 ) return 0;
  z = Strsafe_find(y);
  if( z==0 && (cpy=(char *)lemon_malloc( lemonStrlen(y)+1 ))!=0 ){
    lemon_strcpy(cpy,y);
    z = cpy;
    Strsafe_insert(z);
  }
  MemoryCheck(z);
  return z;
}
/* …（同文件无关代码省略）… */
struct s_x1 {
  int size;               /* The number of available slots. */
                          /*   Must be a power of 2 greater than or */
                          /*   equal to 1 */
  int count;              /* Number of currently slots filled */
  struct s_x1node *tbl;  /* The data stored here */
  struct s_x1node **ht;  /* Hash table for lookups */
};
/* …（同文件无关代码省略）… */
typedef struct s_x1node {
  const char *data;        /* The data */
  struct s_x1node *next;   /* Next entry with the same hash */
  struct s_x1node **from;  /* Previous link */
} x1node;
/* …（同文件无关代码省略）… */
int Strsafe_insert(const char *data)
{
  x1node *np;
  unsigned h;
  unsigned ph;

  if( x1a==0 ) return 0;
  ph = strhash(data);
  h = ph & (x1a->size-1);
  np = x1a->ht[h];
  while( np ){
    if( strcmp(np->data,data)==0 ){
      /* An existing entry with the same key is found. */
      /* Fail because overwrite is not allows. */
      return 0;
    }
    np = np->next;
  }
  if( x1a->count>=x1a->size ){
    /* Need to make the hash table bigger */
    int i,arrSize;
    struct s_x1 array;
    array.size = arrSize = x1a->size*2;
    array.count = x1a->count;
    array.tbl = (x1node*)lemon_calloc(arrSize, sizeof(x1node)+sizeof(x1node*));
    if( array.tbl==0 ) return 0;  /* Fail due to malloc failure */
    array.ht = (x1node**)&(array.tbl[arrSize]);
    for(i=0; i<arrSize; i++) array.ht[i] = 0;
    for(i=0; i<x1a->count; i++){
      x1node *oldnp, *newnp;
      oldnp = &(x1a->tbl[i]);
      h = strhash(oldnp->data) & (arrSize-1);
      newnp = &(array.tbl[i]);
      if( array.ht[h] ) array.ht[h]->from = &(newnp->next);
      newnp->next = array.ht[h];
      newnp->data = oldnp->data;
      newnp->from = &(array.ht[h]);
      array.ht[h] = newnp;
    }
    /* lemon_free(x1a->tbl); // This program was originally for 16-bit machines.
    ** Don't worry about freeing memory on modern platforms. */
    *x1a = array;
  }
  /* Insert the new data */
  h = ph & (x1a->size-1);
  np = &(x1a->tbl[x1a->count++]);
  np->data = data;
  if( x1a->ht[h] ) x1a->ht[h]->from = &(np->next);
  np->next = x1a->ht[h];
  x1a->ht[h] = np;
  np->from = &(x1a->ht[h]);
  return 1;
}
/* …（同文件无关代码省略）… */
const char *Strsafe_find(const char *key)
{
  unsigned h;
  x1node *np;

  if( x1a==0 ) return 0;
  h = strhash(key) & (x1a->size-1);
  np = x1a->ht[h];
  while( np ){
    if( strcmp(np->data,key)==0 ) break;
    np = np->next;
  }
  return np ? np->data : 0;
}
/* …（同文件无关代码省略）… */
struct symbol *Symbol_new(const char *x)
{
  struct symbol *sp;

  sp = Symbol_find(x);
  if( sp==0 ){
    sp = (struct symbol *)lemon_calloc(1, sizeof(struct symbol) );
    MemoryCheck(sp);
    sp->name = Strsafe(x);
    sp->type = ISUPPER(*x) ? TERMINAL : NONTERMINAL;
    sp->rule = 0;
    sp->fallback = 0;
    sp->prec = -1;
    sp->assoc = UNK;
    sp->firstset = 0;
    sp->lambda = LEMON_FALSE;
    sp->destructor = 0;
    sp->destLineno = 0;
    sp->datatype = 0;
    sp->useCnt = 0;
    Symbol_insert(sp,sp->name);
  }
  sp->useCnt++;
  return sp;
}
/* …（同文件无关代码省略）… */
struct s_x2 {
  int size;               /* The number of available slots. */
                          /*   Must be a power of 2 greater than or */
                          /*   equal to 1 */
  int count;              /* Number of currently slots filled */
  struct s_x2node *tbl;  /* The data stored here */
  struct s_x2node **ht;  /* Hash table for lookups */
};
/* …（同文件无关代码省略）… */
typedef struct s_x2node {
  struct symbol *data;     /* The data */
  const char *key;         /* The key */
  struct s_x2node *next;   /* Next entry with the same hash */
  struct s_x2node **from;  /* Previous link */
} x2node;
/* …（同文件无关代码省略）… */
int Symbol_insert(struct symbol *data, const char *key)
{
  x2node *np;
  unsigned h;
  unsigned ph;

  if( x2a==0 ) return 0;
  ph = strhash(key);
  h = ph & (x2a->size-1);
  np = x2a->ht[h];
  while( np ){
    if( strcmp(np->key,key)==0 ){
      /* An existing entry with the same key is found. */
      /* Fail because overwrite is not allows. */
      return 0;
    }
    np = np->next;
  }
  if( x2a->count>=x2a->size ){
    /* Need to make the hash table bigger */
    int i,arrSize;
    struct s_x2 array;
    array.size = arrSize = x2a->size*2;
    array.count = x2a->count;
    array.tbl = (x2node*)lemon_calloc(arrSize, sizeof(x2node)+sizeof(x2node*));
    if( array.tbl==0 ) return 0;  /* Fail due to malloc failure */
    array.ht = (x2node**)&(array.tbl[arrSize]);
    for(i=0; i<arrSize; i++) array.ht[i] = 0;
    for(i=0; i<x2a->count; i++){
      x2node *oldnp, *newnp;
      oldnp = &(x2a->tbl[i]);
      h = strhash(oldnp->key) & (arrSize-1);
      newnp = &(array.tbl[i]);
      if( array.ht[h] ) array.ht[h]->from = &(newnp->next);
      newnp->next = array.ht[h];
      newnp->key = oldnp->key;
      newnp->data = oldnp->data;
      newnp->from = &(array.ht[h]);
      array.ht[h] = newnp;
    }
    /* lemon_free(x2a->tbl); // This program was originally written for 16-bit
    ** machines.  Don't worry about freeing this trivial amount of memory
    ** on modern platforms.  Just leak it. */
    *x2a = array;
  }
  /* Insert the new data */
  h = ph & (x2a->size-1);
  np = &(x2a->tbl[x2a->count++]);
  np->key = key;
  np->data = data;
  if( x2a->ht[h] ) x2a->ht[h]->from = &(np->next);
  np->next = x2a->ht[h];
  x2a->ht[h] = np;
  np->from = &(x2a->ht[h]);
  return 1;
}
/* …（同文件无关代码省略）… */
struct symbol *Symbol_find(const char *key)
{
  unsigned h;
  x2node *np;

  if( x2a==0 ) return 0;
  h = strhash(key) & (x2a->size-1);
  np = x2a->ht[h];
  while( np ){
    if( strcmp(np->key,key)==0 ) break;
    np = np->next;
  }
  return np ? np->data : 0;
}
