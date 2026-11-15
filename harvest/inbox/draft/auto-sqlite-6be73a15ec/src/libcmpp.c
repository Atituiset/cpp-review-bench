// AUTO-DRAFT from sqlite/sqlite PR #cfa787c24d13323b9ec8701f58ed827a52842c31
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR

  ./c-pp -I. -I./src -Dsrcdir=./src -o libcmpp.c ./tool/libcmpp.c-pp.c

  with libcmpp 2.0.x 5b35e14cbfc8ad5202f92e74c5722848594f4321d4db824286a0c99587919ff4 @ 2026-11-14 10:01:36.163 UTC
*/
#if !defined(NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED)
#define NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED
/* …（同文件无关代码省略）… */

  ./c-pp -I. -I./src -Dsrcdir=./src -o libcmpp.h ./tool/libcmpp.c-pp.h

  with libcmpp 2.0.x 5b35e14cbfc8ad5202f92e74c5722848594f4321d4db824286a0c99587919ff4 @ 2026-11-14 10:01:36.163 UTC
*/
#define CMPP_PACKAGE_NAME "libcmpp"
#define CMPP_LIB_VERSION "2.0.x"
#define CMPP_LIB_VERSION_HASH "5b35e14cbfc8ad5202f92e74c5722848594f4321d4db824286a0c99587919ff4"
#define CMPP_LIB_VERSION_TIMESTAMP "2026-11-14 10:01:36.163 UTC"
#define CMPP_LIB_CONFIG_TIMESTAMP "2026-11-14 13:15 GMT"
#define CMPP_VERSION CMPP_LIB_VERSION " " CMPP_LIB_VERSION_HASH " @ " CMPP_LIB_VERSION_TIMESTAMP
#define CMPP_PLATFORM_EXT_DLL ".so"
#define CMPP_MODULE_PATH ".:/usr/local/lib/cmpp"
/* …（同文件无关代码省略）… */
#  define CMPP_EXPORT extern
/* …（同文件无关代码省略）… */
  typedef void cmpp_FILE;
/* …（同文件无关代码省略）… */
typedef struct cmpp__pimpl cmpp__pimpl;
typedef struct cmpp_api_thunk cmpp_api_thunk;
typedef struct cmpp_outputer cmpp_outputer;
typedef struct cmpp_b cmpp_b;
typedef struct cmpp_arg cmpp_arg;
/* …（同文件无关代码省略）… */
typedef uint32_t cmpp_flag32_t;
/* …（同文件无关代码省略）… */
typedef uint32_t cmpp_size_t;
/* …（同文件无关代码省略）… */
typedef int32_t cmpp_strlen_t;
/* …（同文件无关代码省略）… */
#define CMPP_SIZE_T_PFMT PRIu32
/* …（同文件无关代码省略）… */
typedef struct cmpp cmpp;
/* …（同文件无关代码省略）… */
#define cmpp_err_c(PP,RC,MSG) cmpp_err(PP, RC, "" MSG, sizeof(MSG)-1)
/* …（同文件无关代码省略）… */
typedef struct cmpp_errinfo cmpp_errinfo;
/* …（同文件无关代码省略）… */
*/
CMPP_EXPORT bool cmpp_b_chomp(cmpp_b * b);

/**
   Counts the number of newlines between [zBegin,zPos).  zBegin must
   be <= zEnd. zPos must be >= zBegin and <= zEnd. This function may
/* …（同文件无关代码省略）… */
typedef enum cmpp_atpol_e cmpp_atpol_e;
/* …（同文件无关代码省略）… */
typedef enum cmpp_unpol_e cmpp_unpol_e;
/* …（同文件无关代码省略）… */
#define cmpp_tt_map(E)                        \
  E(None,          0,   0)                      \
  E(Error,         1,   0)                    \
  /** Anything with no more specific type. */   \
  E(Word,          2,   cmpp_tt_shape_word)   \
  E(StringSQ,      3,   cmpp_tt_shape_string) \
  E(StringDQ,      4,   cmpp_tt_shape_string) \
  /** Backtick string. */                       \
  E(StringBT,      5,   cmpp_tt_shape_string) \
  /** Decimal int, optionally signed but some
/* …（同文件无关代码省略）… */
typedef enum cmpp_tt_e cmpp_tt_e;
/* …（同文件无关代码省略）… */
typedef struct cmpp_token cmpp_token;
#define cmpp_token_empty_m {.z=0,.n=0,.zInner=0,.nInner=0,.ttype=0}
/* …（同文件无关代码省略）… */
typedef struct cmpp_tizer cmpp_tizer;
/* …（同文件无关代码省略）… */
     "3".

     In non-cmpp_itch invocations of cmpp_f() this is currently always
     0 but The Plan is to eventually offer the ability for directives
     to opt-in to interpolations which most directives already
     currently use. That would save significant amounts of code for
     some directives.

     This buffer is owned by this object but the very simple cmpp_arg
     lifetime model makes it perfectly legal do one of the following
/* …（同文件无关代码省略）… */
typedef struct cmpp_dx cmpp_dx;
typedef struct cmpp__dx_pimpl cmpp__dx_pimpl;
typedef struct cmpp_d cmpp_d;
/* …（同文件无关代码省略）… */
       back into the original source bytes, backslash-escaped newlines
       and all.
  */
  cmpp_f_F_ARGS_V4        = 0x40,

  /**
     Indicates that the cmpp_f() should not be available if the cmpp
/* …（同文件无关代码省略）… */
typedef struct cmpp_f_args cmpp_f_args;
/* …（同文件无关代码省略）… */
#define cmpp_arg_equals_c(ARG,STR) \
  (sizeof("" STR)-1==(ARG)->n && 0==memcmp(STR,(ARG)->z,sizeof(STR)-1))
/* …（同文件无关代码省略）… */
#define cmpp_arg_isflag_c(ARG,STR) \
  (cmpp_arg_equals_c(ARG,STR) || cmpp_arg_equals_c(ARG, "-" STR))
/* …（同文件无关代码省略）… */
   */
  cmpp_arg_interpolate_UNESCAPE      = 0x10,
#endif
#if 0
  /* Going to need this for porting the expression
     builder from itch to the core. */
  cmpp_arg_interpolate_PARENS_EXPR    = 0x20,
#endif

  /**
     TODO? cmpp_arg_interpolate_UNESCAPE to run it through
/* …（同文件无关代码省略）… */

   Returns 0 on success and all that.

   If b is NULL then: (A) if arg->interp is not NULL then it is reused
   as the target else (B) return CMPP_RC_MISUSE and update's pp's
   error state with a description of the problem.

   See: cmpp_atpol_get(), cmpp_atpol_set()

/* …（同文件无关代码省略）… */
typedef struct cmpp_d_reg cmpp_d_reg;
/* …（同文件无关代码省略）… */
typedef struct cmpp_args_pimpl cmpp_args_pimpl;
/* …（同文件无关代码省略）… */
typedef struct cmpp_args cmpp_args;
/* …（同文件无关代码省略）… */
typedef struct cmpp_d_autoloader cmpp_d_autoloader;
/* …（同文件无关代码省略）… */
CMPP_EXPORT bool cmpp_json_valid(cmpp *pp, unsigned char const *z,
                                 cmpp_strlen_t n, int flags);


/**
   Returns true if [z,z+n) begins with '[', ends with ']', and
   cmpp_json_valid() for the same range returns true. i.e. if it's a
/* …（同文件无关代码省略）… */
                                       bool append);

/**
   Appends some form of [z,z+n) to str, using strlen() to calculate n
   if n is negative:

   - 0==n: append "" (and empty JSON string).

   - cmpp_json_valid(z,n) is true: append the bytes as-is.

   - Else append n bytes of z as a JSON-format string using
     sqlite3_str_appendf()'s `%J` formatter.

   This returns void because that's how sqlite3_str's APIs work.  It
   will only fail if str gets "too big" (the limits of which varies by
   str's source but all of which are arguably out of scope for
   cmpp-sized inputs and outputs).
*/
void cmpp_str_json_append(cmpp *pp, sqlite3_str * str,
                          unsigned char const * z,
                          cmpp_strlen_t n);

/**
   A proxy for sqlite3_prepare_v2() which uses pp's database, supports
/* …（同文件无关代码省略）… */
#define cmpp_db_rc(PP,RC,ZMSG) cmpp_db_rc_v2(PP, RC, ZMSG, -1)
/* …（同文件无关代码省略）… */
#    define access(f,m) _access((f),(m))
/* …（同文件无关代码省略）… */
#define CMPP_OMIT_D_INCLUDE
/* …（同文件无关代码省略）… */
#define CMPP_OMIT_D_MODULE
/* …（同文件无关代码省略）… */
#define CMPP__DB_MAIN_NAME "cmpp"
/* …（同文件无关代码省略）… */
#define CMPP_PRIVATE static
/* …（同文件无关代码省略）… */
typedef struct CmppDLine CmppDLine;
/* …（同文件无关代码省略）… */
typedef struct CmppArgList CmppArgList;
/* …（同文件无关代码省略）… */
typedef struct CmppDList_entry CmppDList_entry;
/* …（同文件无关代码省略）… */
#define cmpp__epol(PP,WHICH) (PP)->pimpl->policy.WHICH
#define cmpp__policy(PP,WHICH) \
  cmpp__epol(PP,WHICH).stack[cmpp__epol(PP,WHICH).n]
/* …（同文件无关代码省略）… */
typedef struct cmpp__delim cmpp__delim;
/* …（同文件无关代码省略）… */
static inline cmpp__delim * cmpp__delim_list_get(cmpp__delim_list const *li){
  return li->n ? li->list+(li->n-1) : NULL;
}
/* …（同文件无关代码省略）… */
    /** Name of library-generated savepoints. */
#define CMPP__SAVEPOINT_NAME "_cmpp_"

    /**
       One entry for each distinct query used by cmpp: E(X,SQL), where
       X is the member's name and SQL is its SQL.
/* …（同文件无关代码省略）… */
#define CmppStmt_map(E)                                \
    E(sdefIns,                                         \
      "INSERT INTO "                                   \
      CMPP__DB_MAIN_NAME ".sdef"                       \
      "(t,k,v) VALUES(?1,?2,?3) RETURNING id")         \
    E(defIns,                                          \
      "INSERT INTO "                                   \
      CMPP__DB_MAIN_NAME ".def"                        \
      "(t,k,v) VALUES(?1,?2,?3) "                      \
      "ON CONFLICT(k) DO UPDATE SET v=EXCLUDED.v"      \
    )                                                  \
    E(defDelLegacy,                                    \
      "DELETE FROM "                                   \
      CMPP__DB_MAIN_NAME ".def"                        \
      " WHERE k GLOB ?1")                              \
    E(defDelExact,                                     \
      "DELETE FROM "                                   \
      CMPP__DB_MAIN_NAME ".def"                        \
      " WHERE k =  ?1")                                \
    E(sdefDel,                                         \
      "DELETE FROM "                                   \
      CMPP__DB_MAIN_NAME ".sdef"                       \
      " WHERE k=?1 AND id>=?2")                        \
    E(defHas,                                          \
      "SELECT 1 FROM "                                 \
      CMPP__DB_MAIN_NAME ".vdef"                       \
      " WHERE k = ?1")                                 \
    E(defGet,                                          \
      "SELECT source,t,k,v FROM "                      \
      CMPP__DB_MAIN_NAME ".vdef"                       \
      " WHERE k = ?1 ORDER BY source LIMIT 1")         \
    E(defSelAll, "SELECT t,k,v"                        \
      " FROM " CMPP__DB_MAIN_NAME ".vdef"              \
      " ORDER BY source, k")                           \
    E(inclIns," INSERT OR FAIL INTO "                  \
      CMPP__DB_MAIN_NAME ".incl("                      \
      " file,srcFile, srcLine"                         \
      ") VALUES(?,?,?)")                               \
    E(inclDel, "DELETE FROM "                          \
      CMPP__DB_MAIN_NAME ".incl WHERE file=?")         \
    E(inclHas, "SELECT 1 FROM "                        \
      CMPP__DB_MAIN_NAME ".incl WHERE file=?")         \
    E(inclPathAdd, "INSERT INTO "                      \
      CMPP__DB_MAIN_NAME ".inclpath(priority,dir) "    \
      "VALUES(coalesce(?1,0),?2) "                     \
      "ON CONFLICT DO NOTHING "                        \
      "RETURNING rowid /*xlates to 0 on conflict*/")   \
    E(inclPathRmId, "DELETE FROM "                     \
      CMPP__DB_MAIN_NAME ".inclpath WHERE rowid=?1 "   \
      "RETURNING rowid")                               \
    E(inclSearch,                                      \
      "SELECT ?1 fn WHERE cmpp_file_exists(fn) "       \
      "UNION ALL SELECT fn FROM ("                     \
      " SELECT replace(dir||'/'||?1, '//','/') AS fn " \
      " FROM " CMPP__DB_MAIN_NAME ".inclpath"          \
      " WHERE cmpp_file_exists(fn) "                   \
      " ORDER BY priority DESC, rowid LIMIT 1"         \
      ")")                                             \
    E(dbAttach,                                        \
      "ATTACH ?1 AS ?2")                               \
    E(dbDetach,                                        \
      "DETACH ?1")                                     \
    E(spBegin, "SAVEPOINT " CMPP__SAVEPOINT_NAME)      \
    E(spRollback,                                      \
      "ROLLBACK TO SAVEPOINT " CMPP__SAVEPOINT_NAME)   \
    E(spRelease,                                       \
      "RELEASE SAVEPOINT " CMPP__SAVEPOINT_NAME)       \
    /*E(insTtype,                                      \
/* …（同文件无关代码省略）… */
      "SELECT json_valid(?1,?2)")                      \
    E(jsonSelf,                                        \
      "SELECT json(?1)") /* a.k.a. ?1->'?' */          \
    /*E(jsonOrQuoted,                                  \
      "SELECT if("                                     \
      "json_valid(?1,coalesce(?2,1)),"                 \
      "json(?1),"                                      \
      "?1)"                                            \
    )*/                                                \

#define E(N,S) sqlite3_stmt * N;
    CmppStmt_map(E)
/* …（同文件无关代码省略）… */
enum CmppStmt_e {
  CmppStmt_none = 0,
#define E(N,S) CmppStmt_ ## N,
  CmppStmt_map(E)
#undef E
};
/* …（同文件无关代码省略）… */
static inline cmpp__delim * cmpp__pp_delim(cmpp const *pp){
  return cmpp__delim_list_get(&pp->pimpl->delim.d);
}
static inline char const * cmpp__pp_zdelim(cmpp const *pp){
  cmpp__delim const * const d = cmpp__pp_delim(pp);
  return d ? (char const *)d->open.z : NULL;
}
#define cmpp__dx_delim(DX) cmpp__pp_delim(DX->pp)
#define cmpp__dx_zdelim(DX) cmpp__pp_zdelim(DX->pp)
/* …（同文件无关代码省略）… */
static inline void cmpp__err_reuse(cmpp *pp){
  cmpp_errinfo_reuse(&pp->pimpl->err);
}
/* …（同文件无关代码省略）… */
                                int bindNdx, cmpp_arg const * aVal);

/**
   Helper for #query's bind X part, where aGroup is that X.

   A wrapper around cmpp__bind_arg(). Requires aGroup->ttype to be
   either cmpp_TT_GroupBrace or cmpp_TT_GroupSquiggly and to have
   non-empty content. cmpp_TT_GroupBrace treats it as a list of values
   to bind. cmpp_TT_GroupSquiggly expects sets of 3 tokens per stmt
   column in one of these forms:

   :bindName -> value
   $bindName -> value

   Each LHS refers to a same-named binding in q's SQL, including the
   ':' or '$' prefix. (SQLite supports an '@' prefix but we did not
   historically allow it here to avoid confusion with the now-defunct
   cmpp_TT_StringAt tokens.)

   Each bound value is passed to cmpp__bind_arg() for processing.

   On success, each aGroup entry is bound to q. On error q's state is
   unspecified. Returns ppCode.

   See cmpp__bind_arg() for notes about the bind data type.
*/
CMPP_PRIVATE int cmpp__bind_group(cmpp *pp, sqlite3_stmt * const q,
                                  cmpp_arg const * const aGroup);
/* …（同文件无关代码省略）… */
static inline cmpp_size_t cmpp__strlen(char const *z, cmpp_strlen_t n){
  return n<0 ? (cmpp_size_t)strlen(z) : (cmpp_size_t)n;
}
static inline cmpp_size_t cmpp__strlenu(unsigned char const *z, cmpp_strlen_t n){
  return n<0 ? (cmpp_size_t)strlen((char const *)z) : (cmpp_size_t)n;
}
/* …（同文件无关代码省略）… */
typedef struct cmpp__xval cmpp__xval;
/* …（同文件无关代码省略）… */
typedef struct cmpp__expr cmpp__expr;
/* …（同文件无关代码省略）… */
#define cmpp__fatal(...) cmpp__fatal_base(__FILE__,__LINE__,__VA_ARGS__)
/* …（同文件无关代码省略）… */
#define g_warn(zFmt,...) g_stderr("%s:%d %s() " zFmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
/* …（同文件无关代码省略）… */
#define g_debug(PP,lvl,pfexpr) (void)0
/* …（同文件无关代码省略）… */
#define ustr_c(X) ((unsigned char const *)X)
/* …（同文件无关代码省略）… */
#define dxppCode dx->ppCode
/* …（同文件无关代码省略）… */
#define cmpp__pi(PP) cmpp__pimpl * const pi = PP->pimpl
#define cmpp__dx_pi(DX) cmpp__dx_pimpl * const dpi = DX->pimpl
#define serr(...) cmpp_errf(pp, CMPP_RC_SYNTAX, __VA_ARGS__)
#define dxserr(...) cmpp_errf(dx->pp, CMPP_RC_SYNTAX, __VA_ARGS__)
/* …（同文件无关代码省略）… */
static inline sqlite3_str * cmpp__sqlite3_str_new(cmpp*pp){
  sqlite3_str * const s = sqlite3_str_new(pp ? pp->pimpl->db.dbh : 0);
  return sqlite3_str_errcode(s)
    ? (
      cmpp_check_oom(pp,NULL), NULL
      /* s is a static singleton in this case, not leaked */
    )
    : s;
}
/* …（同文件无关代码省略）… */
#define cmpp__err cmpp_errf
#define cmpp_dx_err cmpp_dx_errf
/* …（同文件无关代码省略）… */
typedef struct FileWrapper FileWrapper;
/* …（同文件无关代码省略）… */
cmpp_FILE * cmpp_fopen(const char *zName, const char *zMode){
  cmpp_FILE *f;
  if(zName && ('-'==*zName && !zName[1])){
    f = (strchr(zMode, 'w') || strchr(zMode,'+'))
      ? stdout
      : stdin
      ;
  }else{
    f = fopen(zName, zMode);
  }
  return f;
}
/* …（同文件无关代码省略）… */
void cmpp_fclose( cmpp_FILE * f ){
  if(f && (stdin!=f) && (stdout!=f) && (stderr!=f)){
    fclose(f);
  }
}
/* …（同文件无关代码省略）… */
void FileWrapper_close(FileWrapper * p){
  if(p->pFile) cmpp_fclose(p->pFile);
  if(p->zContent) cmpp_mfree(p->zContent);
  *p = FileWrapper_empty;
}
/* …（同文件无关代码省略）… */
int FileWrapper_open(FileWrapper * p, const char * zName,
                     const char * zMode){
  FileWrapper_close(p);
  if( (p->pFile = cmpp_fopen(zName, zMode)) ){
    p->zName = zName;
    return 0;
  }else{
    return cmpp_errno_rc(errno, CMPP_RC_IO);
  }
}
/* …（同文件无关代码省略）… */
int FileWrapper_slurp(FileWrapper * p, int bCloseFile){
  assert(!p->zContent);
  assert(p->pFile);
  int const rc = cmpp_slurp(cmpp_input_f_FILE, p->pFile,
                            &p->zContent, &p->nContent);
  if( bCloseFile ){
    cmpp_fclose(p->pFile);
    p->pFile = 0;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static inline cmpp_d const * cmpp_dx_d(cmpp_dx const * const dx){
  return dx->d;
}
/* …（同文件无关代码省略）… */
int cmpp__lazy_init(cmpp *pp){
  if( !ppCode && pp->pimpl->flags.needsLazyInit ){
    cmpp__pimpl * const pi = pp->pimpl;
    pi->flags.needsLazyInit = false;
    cmpp__delim_list * li = &pi->delim.d;
    if( !li->n ) cmpp_delimiter_push(pp, NULL);
    li = &pi->delim.at;
    if( !li->n ) cmpp_atdelim_push(pp, NULL, NULL);
#if defined(CMPP_CTOR_INSTANCE_INIT)
    if( !ppCode ){
      extern int CMPP_CTOR_INSTANCE_INIT(cmpp*);
      int const rc = CMPP_CTOR_INSTANCE_INIT(pp);
      if( rc && !ppCode ){
        cmpp__err(pp, rc,
                  "Initialization via CMPP_CTOR_INSTANCE_INIT() failed "
                  "with code %d/%s.", rc, cmpp_rc_cstr(rc) );
      }
    }
#endif
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static int cmpp__FileWrapper_open(cmpp *pp, FileWrapper * fw,
                                  const char * zName,
                                  const char * zMode){
  int const rc = FileWrapper_open(fw, zName, zMode);
  if( rc ){
    cmpp__err(pp, rc, "Error %s opening file [%s] "
                  "with mode [%s]",
                  cmpp_rc_cstr(rc), zName, zMode);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static int cmpp__FileWrapper_slurp(cmpp* pp, FileWrapper * fw){
  assert( fw->pFile );
  int const rc = FileWrapper_slurp(fw, 1);
  if( rc ){
    cmpp__err(pp, rc, "Error %s slurping file %s",
              cmpp_rc_cstr(rc), fw->zName);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
void g_stderrv(char const *zFmt, va_list va){
  vfprintf(0 ? stdout : stderr, zFmt, va);
}
/* …（同文件无关代码省略）… */
void g_stderr(char const *zFmt, ...){
  va_list va;
  va_start(va, zFmt);
  g_stderrv(zFmt, va);
  va_end(va);
}
/* …（同文件无关代码省略）… */
int cmpp__out2(cmpp *pp, cmpp_outputer *pOut,
               void const *z, cmpp_size_t n){
  assert( pOut );
  if( !ppCode && pOut->out && n ){
    int const rc = pOut->out(pOut->state, z, n);
    if( rc ){
      cmpp__err(pp, rc,
                "Write of %" CMPP_SIZE_T_PFMT
                " bytes to output stream failed.", n);
    }
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static int cmpp__affirm_undef_policy(cmpp *pp,
                                     unsigned char const *zName,
                                     cmpp_size_t nName){
  if( 0==ppCode
      && cmpp_unpol_ERROR==cmpp__policy(pp,un) ){
    cmpp__err(pp, CMPP_RC_NOT_DEFINED,
              "Key '%.*s' was not found and the undefined-value "
              "policy is 'error'.",
              (int)nName, zName);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
void cmpp__outputer_swap(cmpp *pp, cmpp_outputer const *oNew,
                        cmpp_outputer *oPrev){
  if( oPrev ){
    *oPrev = pp->pimpl->out;
  }
  pp->pimpl->out = *oNew;
}
/* …（同文件无关代码省略）… */
int cmpp__has(cmpp *pp, const char * zName, cmpp_strlen_t nName){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defHas, false);
  if( q ){
    nName = cmpp__strlen(zName, nName);
    cmpp_stmt_bind_textn(pp, q, 1, ustr_c(zName), nName);
    if(SQLITE_ROW == cmpp_stmt_step(pp, q, true)){
      rc = 1;
    }else{
      rc = 0;
    }
    //g_debug(pp,1,("has [%s] ?= %d\n",zName, rc));
  }
  return rc;
}
/* …（同文件无关代码省略）… */
int cmpp__get_b(cmpp *pp, unsigned const char * zName,
                cmpp_strlen_t nName, cmpp_b * b,
                bool enforceUndefPolicy){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defGet, false);
  if( !q ) return 0;
  nName = cmpp__strlenu(zName, nName);
  cmpp_stmt_bind_textx(pp, q, 1, zName, nName, SQLITE_STATIC);
  int n = 0;
  if(SQLITE_ROW == cmpp_stmt_step(pp, q, false)){
    const unsigned char * z = sqlite3_column_blob(q, 3);
    n = sqlite3_column_bytes(q, 3);
    if( 0==cmpp_b_append4(pp, b, z, (cmpp_size_t)n) ){
      rc = 1;
    }
  }
  cmpp_stmt_reset(pp, q);
  g_debug(pp,1,("get-define [%.*s] ?= %d %.*s\n",
                nName, zName, rc, b->n, b->z));
  if( enforceUndefPolicy && !rc ){
    cmpp__affirm_undef_policy(pp, zName, nName);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
bool cmpp__is_legal_key(unsigned char const *zName,
                        cmpp_size_t n,
                        unsigned char const **zErrPos,
                        bool equalIsLegal){
  if( !n || n>64/*arbitrary*/ ){
    if( zErrPos ) *zErrPos = 0;
    return false;
  }
  unsigned char const * z = zName;
  unsigned char const * const zEnd = zName + n;
  for( ; z<zEnd; ++z ){
    if( !((*z>='a' && *z<='z')
          || (*z>='A' && *z<='Z')
          || (z>zName &&
              ('-'==*z
               /* This is gonna bite us if we extend the expresions to
                  support +/-. Expressions currently parse X=Y (no
                  spaces) as the three tokens X = Y, but we'd need to
                  require a space between X-Y in expressions because
                  '-' is a legal symbol character. i've looked at
                  making '-' illegal but it's just too convenient for
                  use in define keys.  Once one is used to
                  tcl-style-naming of stuff, it's painful to have to go
                  back to snake_case.
               */
               || (*z>='0' && *z<='9')))
          || (*z>='.' && *z<='/')
          || (*z==':')
          || (*z=='_')
          || (equalIsLegal && z>zName && '='==*z)
          || (*z & 0x80)
        ) ){
      if( zErrPos ) *zErrPos = z;
      return false;
    }
  }
  return true;
}
/* …（同文件无关代码省略）… */
int cmpp__legal_key_check(cmpp *pp, unsigned char const *zKey,
                          cmpp_strlen_t nKey, bool permitEqualSign){
  if( !ppCode ){
    unsigned char const *zAt = 0;
    nKey = cmpp__strlenu(zKey, nKey);
    if( !cmpp__is_legal_key(zKey, nKey, &zAt, permitEqualSign) ){
      cmpp__err(pp, CMPP_RC_SYNTAX,
                "Illegal character 0x%02x in key [%.*s]",
                (int)*zAt, nKey, zKey);
    }
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static cmpp_size_t cmpp__dx_guess_lineno(cmpp_dx const * dx,
                                         unsigned char const * zWhere){
  cmpp__dx_pi(dx);
  cmpp_size_t line = 0;
  if( !zWhere ) zWhere = dpi->zErrPos;
  if( !zWhere ){
    zWhere = dpi->pos.z;
    if( zWhere && dpi->dline.zEnd && zWhere>dpi->dline.zBegin ){
      /* dpi->pos.z is normally just past the current directive line's
         EOL, i.e. at the start of the next line. In that case we're
         still on the directive's line, and reporting the next line
         would be misleading. */
      unsigned char const * z = dpi->dline.zEnd;
      if( z<zWhere && '\r'==*z ) ++z;
      if( z<zWhere && '\n'==*z ) ++z;
      if( zWhere<=z ) return dpi->dline.lineNo;
    }
  }
  return cmpp__dx_where(dx, zWhere, &line, NULL)
    ? line : dpi->dline.lineNo;
}
/* …（同文件无关代码省略）… */
static cmpp_dx const * cmpp__dx_input_owner(cmpp_dx const * dx){
  cmpp_dx const * up;
  while( (up = dx->pimpl->up) ){
    /* If up lives in the same byte range, use it for purposes of
       figuring out the source range. */
    if( dx->pimpl->zBegin>=up->pimpl->zBegin
        && dx->pimpl->zEnd<=up->pimpl->zEnd ){
      dx = up;
    }else{
      break;
    }
  }
  return dx;
}
/* …（同文件无关代码省略）… */
bool cmpp__dx_where(cmpp_dx const * dx, unsigned char const * z,
                    cmpp_size_t * pLine, cmpp_size_t * pCol){
  cmpp__dx_pi(dx);
  if( !z ) return false;
  cmpp_dx const * const owner = cmpp__dx_input_owner(dx);
  cmpp__dx_pimpl const * const opi = owner->pimpl;
  cmpp_size_t col = 0, nl;
  if( opi->zBegin && z>=opi->zBegin && z<=opi->zEnd ){
    nl = cmpp_count_nl(opi->zBegin, opi->zEnd, z, &col);
    *pLine = 1 + nl + opi->origin.lineBase;
    if( pCol ) *pCol = col + (nl ? 0 : opi->origin.colBase);
    return true;
  }
  cmpp_b const * const bl = &dpi->buf.line;
  if( bl->z && z>=bl->z && z<=bl->z+bl->n
      && dpi->dline.zBegin && z!=dpi->dline.zBegin/*recursion guard*/ ){
    /* buf.line is a copy of the directive line, starting at
       dline.zBegin, with the backslashes of backslash-escaped
       newlines removed. That preserves its newlines and, because
       those backslashes are always at the end of a line, its
       columns. */
    cmpp_size_t dLine = 0, dCol = 0;
    if( !cmpp__dx_where(dx, dpi->dline.zBegin, &dLine, &dCol) ){
      return false;
    }
    nl = cmpp_count_nl(bl->z, bl->z+bl->n, z, &col);
    *pLine = dLine + nl;
    if( pCol ) *pCol = nl ? col : dCol + col;
    return true;
  }
  return false;
}
/* …（同文件无关代码省略）… */
int cmpp__args_tzerr_uplift(cmpp * const pp, cmpp_tizer const * const tz){
  return pp->pimpl->dx
    ? cmpp__dx_tzerr_uplift(pp->pimpl->dx, tz)
    : ppCode;
}
/* …（同文件无关代码省略）… */
int cmpp__dx_skip(cmpp_dx * const dx,
                  cmpp_d const * const * const dClosers,
                  unsigned nClosers){
  if( dxppCode ) return dxppCode;
  cmpp__dx_pimpl * const dpi = dx->pimpl;
  cmpp_d const * const d = cmpp_dx_d(dx);
  cmpp_size_t const lineNo = dpi->dline.lineNo;
  bool const wasSkipping = dpi->flags.skipping;
  bool gotOne = false;
  dpi->flags.skipping = true;
  while( 0==dxppCode
         && 0==cmpp_dx_next(dx, &gotOne) ){
    if( !gotOne ){
      dxserr("No closing directive found for "
             "%s%s opened on line %" CMPP_SIZE_T_PFMT ".",
             cmpp_dx_delim(dx), d->name.z, lineNo);
      break;
    }
    cmpp_d const * const d2 = cmpp_dx_d(dx);
    unsigned i = 0;
    for( ; i < nClosers && d2!=dClosers[i]; ++i ){}
    if( i < nClosers ){
      break;
    }else if( cmpp_f_F_BLOCK_OPEN & d2->flags ){
      assert( d2->closer );
      cmpp__dx_skip(dx, &d2->closer, 1);
    }else if( cmpp_f_F_BLOCK_CLOSE & d2->flags ){
      dxserr("Unexpected %s%s inside %s%s opened on line %"
             CMPP_SIZE_T_PFMT ".",
             cmpp_dx_delim(dx), d2->name.z,
             cmpp_dx_delim(dx), d->name.z, lineNo);
    }
    /* Else ignore it. */
  }
  dpi->flags.skipping = wasSkipping;
  return dxppCode;
}
/* …（同文件无关代码省略）… */
char const * cmpp__atpol_name(cmpp *pp, cmpp_atpol_e p){
again:
  switch(p){
    case cmpp_atpol_CURRENT:{
      if( pp ){
        assert( p!=cmpp__policy(pp, at) );
        p = cmpp__policy(pp, at);
        pp = 0;
        goto again;
      }
      return NULL;
    }
    case cmpp_atpol_invalid: return NULL;
    case cmpp_atpol_OFF: return "off";
    case cmpp_atpol_RETAIN: return "retain";
    case cmpp_atpol_ELIDE: return "elide";
    case cmpp_atpol_ERROR: return "error";
  }
  return NULL;
}
/* …（同文件无关代码省略）… */
cmpp_atpol_e cmpp_atpol_from_str(cmpp * const pp, char const *z){
  cmpp_atpol_e rv = cmpp_atpol_invalid;
  if( 0==strcmp(z,      "retain") )  rv = cmpp_atpol_RETAIN;
  else if( 0==strcmp(z, "elide") )   rv = cmpp_atpol_ELIDE;
  else if( 0==strcmp(z, "error") )   rv = cmpp_atpol_ERROR;
  else if( 0==strcmp(z, "off") )     rv = cmpp_atpol_OFF;
  if( pp ){
    if( cmpp_atpol_invalid==rv
        && 0==strcmp(z, "current") ){
      rv = cmpp__policy(pp,at);
    }else if( cmpp_atpol_invalid==rv ){
      cmpp__err(pp, CMPP_RC_RANGE,
                "Invalid @ policy value: %s."
                " Try one of retain|elide|error|off|current.", z);
    }else{
      cmpp__policy(pp,at) = rv;
    }
  }
  return rv;
}
/* …（同文件无关代码省略）… */
int cmpp_atpol_push(cmpp * pp, cmpp_atpol_e pol){
  if( cmpp_atpol_CURRENT==pol ) pol = cmpp__policy(pp,at);
  assert( cmpp_atpol_CURRENT!=pol && "Else internal mismanagement." );
  if( 0==PodList__atpol_push(pp, &cmpp__epol(pp,at), pol)
      && 0!=cmpp_atpol_set(pp, pol)/*for validation*/ ){
    PodList__atpol_pop(&cmpp__epol(pp,at));
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
void cmpp_atpol_pop(cmpp * pp){
  assert( cmpp__epol(pp,at).n );
  if( cmpp__epol(pp,at).n ){
    PodList__atpol_pop(&cmpp__epol(pp,at));
  }else if( !ppCode ){
    cmpp_errf(pp, CMPP_RC_MISUSE,
                 "%s() called when no cmpp_atpol_push() is active.",
                 __func__);
  }
}
/* …（同文件无关代码省略）… */
int cmpp_unpol_push(cmpp * pp, cmpp_unpol_e pol){
  if( 0==PodList__unpol_push(pp, &cmpp__epol(pp,un), pol)
      && cmpp_unpol_set(pp, pol)/*for validation*/ ){
    PodList__unpol_pop(&cmpp__epol(pp,un));
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
void cmpp_unpol_pop(cmpp * pp){
  assert( cmpp__epol(pp,un).n );
  if( cmpp__epol(pp,un).n ){
    PodList__unpol_pop(&cmpp__epol(pp,un));
  }else if( !ppCode ){
    cmpp_errf(pp, CMPP_RC_MISUSE,
                 "%s() called when no cmpp_unpol_push() is active.",
                 __func__);
  }
}
/* …（同文件无关代码省略）… */
int cmpp__dx_sp_begin(cmpp_dx * const dx){
  if( 0==dxppCode && 0==cmpp_sp_begin(dx->pp) ){
    ++dx->pimpl->nSavepoint;
  }
  return dxppCode;
}
/* …（同文件无关代码省略）… */
int cmpp__dx_sp_rollback(cmpp_dx * const dx){
  /* Remember that rollback must (mostly) ignore the pending error state. */
  if( !dx->pimpl->nSavepoint ){
    if( 0==dxppCode ){
      cmpp_dx_err(dx, CMPP_RC_MISUSE,
                  "Cannot roll back: no active savepoint");
    }
  }else{
    cmpp_sp_rollback(dx->pp);
    --dx->pimpl->nSavepoint;
  }
  return dxppCode;
}
/* …（同文件无关代码省略）… */
int cmpp__dx_sp_commit(cmpp_dx * const dx){
  if( 0==dxppCode ){
    if( !dx->pimpl->nSavepoint ){
      cmpp_dx_err(dx, CMPP_RC_MISUSE,
                  "Cannot commit: no active savepoint");
    }else if( 0==cmpp_sp_commit(dx->pp) ){
      --dx->pimpl->nSavepoint;
    }
  }
  return dxppCode;
}
/* …（同文件无关代码省略）… */
int cmpp_process_file(cmpp *pp, const char * zName){
  if( 0==ppCode ){
    FileWrapper fw = FileWrapper_empty;
    if( 0==cmpp__FileWrapper_open(pp, &fw, zName, "rb")
        && 0==cmpp__FileWrapper_slurp(pp, &fw) ){
      cmpp_process_string(pp, zName, fw.zContent, fw.nContent);
    }
    FileWrapper_close(&fw);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
int cmpp_input_f_FILE( void * state, void * dest, cmpp_size_t * n ){
  if( !*n ) return CMPP_RC_RANGE;
  cmpp_FILE * f = (cmpp_FILE*) state;
  cmpp_size_t const rn = *n;
  *n = (size_t)fread(dest, 1, rn, f);
  if(*n==rn || feof(f)) return 0;
  else{
    int const eno = errno;
    return eno ? cmpp_errno_rc(eno, CMPP_RC_IO) : CMPP_RC_IO;
  }
}
/* …（同文件无关代码省略）… */
void cmpp__fatalv_base(char const *zFile, int line,
                  char const *zFmt, va_list va){
  cmpp_FILE * const fp = stderr;
  fflush(stdout);
  fprintf(fp, "\n%s:%d: ", zFile, line);
  if(zFmt && *zFmt){
    vfprintf(fp, zFmt, va);
    fputc('\n', fp);
  }
  fflush(fp);
  exit(1);
}
/* …（同文件无关代码省略）… */
void cmpp__fatal_base(char const *zFile, int line,
                      char const *zFmt, ...){
  va_list va;
  va_start(va, zFmt);
  cmpp__fatalv_base(zFile, line, zFmt, va);
  va_end(va);
}
/* …（同文件无关代码省略）… */
void cmpp__dx_append_script_info(cmpp_dx const * dx,
                                 sqlite3_str * const sstr){
  cmpp__dx_pi(dx);
  //g_warn("dpi->zErrPos: %p\n%.3s ...", dpi->zErrPos, dpi->zErrPos);
  //g_warn("dpi->pos.z: %p\n %.3s ...", dpi->pos.z, dpi->pos.z);
  cmpp_size_t const nl = cmpp__dx_guess_lineno(dx, 0);
  ///g_warn("nl=%d", (int)nl);
  char const * zSrcName =
    (dx->sourceName
     && 0==strcmp("-", (char const *)dx->sourceName))
    ? "<stdin>"
    : (char const *)dx->sourceName;
  cmpp_size_t atLine = 0, atCol = 0;
  bool const haveAt = dpi->zErrAt
    && cmpp__dx_where(dx, dpi->zErrAt, &atLine, &atCol);
  if( dx->d ){
    if( haveAt ){
      /* We know precisely where the error is. */
      sqlite3_str_appendf(sstr,  "%s @ %s:%" CMPP_SIZE_T_PFMT
                          ":%" CMPP_SIZE_T_PFMT,
                          dx->d->name.z, zSrcName, atLine, atCol);
      if( atLine != dpi->dline.lineNo ){
        sqlite3_str_appendf(sstr, " (directive on line %"
                            CMPP_SIZE_T_PFMT ")", dpi->dline.lineNo);
      }
    }else{
      sqlite3_str_appendf(sstr,  "%s @ %s:%" CMPP_SIZE_T_PFMT,
                          dx->d->name.z, zSrcName,
                          dpi->dline.lineNo);
      if( nl != dpi->dline.lineNo ){
        /* Input position past the directive line. */
        sqlite3_str_appendf(sstr, " from line %" CMPP_SIZE_T_PFMT, nl);
      }
    }
  }else if( haveAt ){
    sqlite3_str_appendf(sstr,  "%s:%" CMPP_SIZE_T_PFMT
                        ":%" CMPP_SIZE_T_PFMT,
                        zSrcName, atLine, atCol);
  }else{
    sqlite3_str_appendf(sstr,  "%s:%" CMPP_SIZE_T_PFMT,
                        zSrcName, nl);
  }
}
/* …（同文件无关代码省略）… */
int cmpp_errfv(cmpp *pp, int rc, char const *zFmt, va_list va){
  if( pp ){
    cmpp_errinfo * const err = &pp->pimpl->err;
    cmpp_errinfo_reuse(err);
    assert( 0==err->code )
      /* some APIs become no-ops if pp->pimpl->err.code
         is set, so delay setting this. */;
    if( 0==rc ) return rc;
    if( CMPP_RC_OOM==rc ){
    oom:
      err->zStatic ="An allocation failed.";
      return err->code = CMPP_RC_OOM;
    }
    assert( !err->zStatic );
    if( pp->pimpl->dx || (zFmt && *zFmt) ){
      sqlite3_str * const sstr = cmpp__sqlite3_str_new(pp);
      if( !sstr ) goto oom;
      if( pp->pimpl->dx ){
        cmpp__dx_append_script_info(pp->pimpl->dx, sstr);
        sqlite3_str_append(sstr, ": ", 2);
      }
      if( zFmt && *zFmt ){
        sqlite3_str_vappendf(sstr, zFmt, va);
      }else{
        sqlite3_str_appendf(sstr, "No error info provided.");
      }
      int nz = 0;
      char * const z = cmpp_sqlite3_str_finish(pp, sstr, &nz);
      //g_warn("dx =%p zFmt=%s z=%s", pp->pimpl->dx, zFmt, z);
      if( !z ) goto oom;
#if 0
      cmpp_b_append(&err->b, z, nz);
      sqlite3_free(z);
#else
      if( err->b.z ){
        if( cmpp_b_append4_str(pp, &err->b, z, nz) ){
          sqlite3_free(z);
          goto oom;
        }
        sqlite3_free(z);
      }else{
        /* Transfer ownership. */
        err->b.z = (unsigned char*)z;
        err->b.n = (cmpp_size_t)nz;
        err->b.nAlloc = err->b.n + 1 /*NUL terminator*/;
      }
#endif
      err->zStatic = (char *)err->b.z;
    }else{
      err->zStatic = "No error info provided.";
    }
    err->code = rc;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
int cmpp_errf(cmpp *pp, int rc,
                 char const *zFmt, ...){
  if( pp ){
    va_list va;
    va_start(va, zFmt);
    //rc = cmpp_errinfo_setv(&pp->pimpl->err, rc, zFmt, va);
    rc = cmpp_errfv(pp, rc, zFmt, va);
    va_end(va);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
int cmpp_dx_errf(cmpp_dx *dx, int rc,
                    char const *zFmt, ...){
  if( rc ){
    if( !dx->pimpl->zErrPos ){
      dx->pimpl->zErrPos = dx->pimpl->dline.zBegin;
      //g_warn("dx->pimpl->dline.zBegin: %.3s...", dx->pimpl->dline.zBegin);
      if( !dx->pimpl->zErrPos ) dx->pimpl->zErrPos = dx->pimpl->zBegin;
      //g_warn("dx->pimpl->zBegin: %.3s...", dx->pimpl->zBegin);
    }
  }else{
    dx->pimpl->zErrPos = 0;
  }
  va_list va;
  va_start(va, zFmt);
  rc = cmpp_errfv(dx->pp, rc, zFmt, va);
  va_end(va);
  return rc;
}
/* …（同文件无关代码省略）… */
bool cmpp__arg_is_flag( char const *zFlag, char const *zArg,
                        char const **zValIfEqX ){
  if( zValIfEqX ) *zValIfEqX = 0;
  if( 0==strcmp(zFlag, zArg) ) return true;
  char const * z = strchr(zArg,'=');
  if( z && z>zArg ){
    /* compare the part before the '=' */
    if( 0==strncmp(zFlag, zArg, z-zArg) ){
      if( !zFlag[z-zArg] ){
        if( zValIfEqX ) *zValIfEqX = z+1;
        return true;
      }
      /* Else it was a prefix match. */
    }
  }
  return false;
}
/* …（同文件无关代码省略）… */
  }
}

CMPP__EXPORT(bool, cmpp_json_valid)(cmpp *pp, unsigned char const *z,
                                    cmpp_strlen_t n, int flags){
  bool rv = false;
  n = cmpp__strlenu(z, n);
  if( n && flags>0 && flags<16 ){
    /* ^^^^ json_valid() is picky about its flags argument.  We skip
       processing for that case because it can put pp into an error
       state which the caller is not likely to check for because who
       would after such an innocuous function? */
    sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_jsonValid, false);
    if( q ){
      cmpp_stmt_bind_textx(pp, q, 1, z, n, SQLITE_STATIC);
      cmpp_stmt_bind_int(pp, q, 2, flags);
      if( SQLITE_ROW==cmpp_stmt_step(pp, q, false) ){
        rv = 0!=sqlite3_column_int(q, 0);
      }
      cmpp_stmt_reset(pp, q);
    }
  }
  return rv;
}

static inline bool cmpp__json_is_container(cmpp * const pp,
                                           char opener,
                                           char closer,
                                           unsigned char const * const z,
                                           cmpp_strlen_t n, bool allowJson5){
  n = cmpp__strlenu(z, n);
  return (
    n>1
    && z[0]==(unsigned char)opener
    && z[n-1]==(unsigned char)closer
  ) ? (
    n==2
    ? true
    : cmpp_json_valid(pp, z, n, allowJson5 ? 3 : 1)
  ) : false;
}

CMPP__EXPORT(bool,cmpp_json_is_list)(cmpp *pp,
                                     unsigned char const * const z,
                                     cmpp_strlen_t n, bool allowJson5){
  return cmpp__json_is_container(pp, '[', ']', z, n, allowJson5);
}

CMPP__EXPORT(bool,cmpp_json_is_object)(cmpp *pp,
                                       unsigned char const * const z,
                                       cmpp_strlen_t n, bool allowJson5){
  return cmpp__json_is_container(pp, '{', '}', z, n, allowJson5);
}

CMPP__EXPORT(int, cmpp_json_key_to_path)(cmpp *pp, int mode,
                                         unsigned char const *z,
                                         cmpp_strlen_t n, cmpp_b * bKey){
  if( ppCode ) return ppCode;
  n = cmpp__strlenu(z, n);
  if( !n ){
    return cmpp_b_append4(pp, bKey, mode
                          ? "$[]"
                          : "$.\0"/*illegal, but let SQLite report that*/,
                          3);
  }
  if( '$'==z[0] && (1==n || (mode ? '['==z[1] : '.'==z[1])) ){
    return cmpp_b_append4(pp, bKey, z, n);
  }
  /* This does not superficially look like a JSON path prefix, make it
     look like one. */
  unsigned char const * zz = 0;
  cmpp_size_t nn = 0;
  if( '$'==z[0] && '.'==z[1] ){
    zz = z+2;
    nn = n - 2;
  }else{
    zz = z;
    nn = n;
  }
  cmpp_b_append(bKey, mode ? "$[" : "$.", 2);
  cmpp_b_append(bKey, zz, nn);
  if( mode ) cmpp_b_append_ch(bKey, ']');
  if( bKey->errCode ) cmpp_err(pp, bKey->errCode, 0, 0);
  return ppCode;
}

CMPP__EXPORT(void,cmpp_str_json_append)(cmpp *pp, sqlite3_str * str,
                                        unsigned char const * z,
                                        cmpp_strlen_t n){
  n = cmpp__strlenu(z, n);
  if( !n ){
    sqlite3_str_append(str, "\"\"", 2);
    return;
  }
  int64_t j;
  if( cmpp_is_int64(z, n, &j) ){
    sqlite3_str_appendf(str, "%lld", j);
  }else{
    /* If it's JSON then append it as-is, don't escape it.  We
       "could" handle ints here but it's much faster to do them
       above. */
    //g_warn("arg %d: %.*s", (int)i, (int)n, z);
    if( cmpp_json_valid(pp, z, n, 1) ){
      /* Using any json_valid() flags which allow json5 will end
         up generating JSON5 output, which we don't want here
         because it would probably give us headaches everywhere
         else. e.g. [list new a b c] would result in [a, b, c]
         instead of ["a", "b", "c"]. */
      //g_warn("is-valid 1: %.*s", (int)n, z);
      sqlite3_str_append(str, (char *)z, (int)n);
    }else{
      //g_warn("is-valid 0: %.*s", (int)n, z);
      sqlite3_str_appendf(str, "%.*J", (int)n, z);
    }
  }
}

CMPP__EXPORT(int, cmpp_json_canonicalize)(cmpp *pp, unsigned char const *z,
                                          cmpp_size_t n, cmpp_b *b,
                                          bool append){
  sqlite3_stmt * const q =
    cmpp__stmt(pp, CmppStmt_jsonSelf, false);
  if( !q ) return ppCode;
  cmpp_stmt_bind_textx(pp, q, 1, z, n, SQLITE_STATIC);
  if( SQLITE_ROW==cmpp_stmt_step(pp, q, false) ){
    z = sqlite3_column_blob(q,0);
    n = sqlite3_column_bytes(q,0);
    //g_warn("canon=%.*s", (int)n, z);
    cmpp_b_append4(pp, append ? b : cmpp_b_reuse(b), z, n);
  }
  cmpp_stmt_reset(pp, q);
  return ppCode;
}

void cmpp__dump_defines(cmpp *pp, cmpp_FILE * fp, int bIndent){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defSelAll, false);
  if( q ){
/* …（同文件无关代码省略）… */
#define M(X) cmpp__arg_is_flag(X, zArg, &zVal)
/* …（同文件无关代码省略）… */
#define PC 0x80 /* pad character */
#define WS 0x81 /* whitespace */
#define ND 0x82 /* Not above or digit-value */
#define PAD_CHAR '='
/* …（同文件无关代码省略）… */
#define BX_NUMERAL(dv) (b64Numerals[(uint8_t)(dv)])
/* …（同文件无关代码省略）… */
#define B64_DARK_MAX 72
/* …（同文件无关代码省略）… */
static char* toBase64( uint8_t const *pIn, cmpp_size_t nbIn, char *pOut ){
  int nCol = 0;
  while( nbIn >= 3 ){
    /* Do the bit-shuffle, exploiting unsigned input to avoid masking. */
    pOut[0] = BX_NUMERAL(pIn[0]>>2);
    pOut[1] = BX_NUMERAL(((pIn[0]<<4)|(pIn[1]>>4))&0x3f);
    pOut[2] = BX_NUMERAL(((pIn[1]&0xf)<<2)|(pIn[2]>>6));
    pOut[3] = BX_NUMERAL(pIn[2]&0x3f);
    pOut += 4;
    nbIn -= 3;
    pIn += 3;
    if( (nCol += 4)>=B64_DARK_MAX || nbIn<=0 ){
      *pOut++ = '\n';
      nCol = 0;
    }
  }
  if( nbIn > 0 ){
    signed char nco = nbIn+1;
    cmpp_strlen_t nbe;
    unsigned long qv = *pIn++;
    for( nbe=1; nbe<3; ++nbe ){
      qv <<= 8;
      if( (cmpp_size_t)nbe<nbIn ) qv |= *pIn++;
    }
    for( nbe=3; nbe>=0; --nbe ){
      char ce = (nbe<nco)? BX_NUMERAL((uint8_t)(qv & 0x3f)) : PAD_CHAR;
      qv >>= 6;
      pOut[nbe] = ce;
    }
    pOut += 4;
    *pOut++ = '\n';
  }
  *pOut = 0;
  return pOut;
}
/* …（同文件无关代码省略）… */
int cmpp__b_base64_encode(cmpp *pp, cmpp_b const * bIn, cmpp_b * bOut){
  cmpp_size_t nc;
  cmpp_size_t const nv = bIn->n;
  char *cBuf;
  cmpp_size_t const nvMax = 1000 * 1000 * 1000
    /* 1B == default SQLITE_MAX_LENGTH, but that symbol is not exposed
       in its public API. We "could" extract that limit from pp's db
       here, but... no. We're only using it as a guideline: bOut is
       not not part of the db state so we're not beholden to the db's
       limits.  In this context the limit is arbitrary, anyway - we
       inherit it from this code's origins and retain it for
       simplicity. */;
  if( ppCode ) return ppCode;
  nc = 4*((nv+2)/3); /* quads needed */
  nc += (nc+(B64_DARK_MAX-1))/B64_DARK_MAX + 1; /* LFs and a 0-terminator */
  if( nvMax < nc ){
    return cmpp_errf(pp, CMPP_RC_RANGE,
                         "Blob expanded to base64 is too big.");
  }
  cmpp_b_reuse(bOut);
  if( !bIn->z || !bIn->n ){
    /* empty is okay */
    return 0;
  }
  if( 0==cmpp_b_reserve3(pp, bOut, nc) ){
    cBuf = (char*)bOut->z;
    bOut->n = (cmpp_size_t)(toBase64(bIn->z, bIn->n, cBuf) - cBuf);
    assert( bOut->n < bOut->nAlloc );
    cBuf[bOut->n] = 0;
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
}


#undef PC
#undef WS
#undef ND
/* …（同文件无关代码省略）… */
static int cmpp__stmt_prepare(cmpp *pp, bool honorError,
                              sqlite3_stmt **pStmt,
                              const char * zSql, cmpp_strlen_t nSql){
  if( honorError && ppCode ) return ppCode;
  nSql = cmpp__strlen(zSql, nSql);
  int const rc = sqlite3_prepare_v2(pp->pimpl->db.dbh, zSql, nSql, pStmt, 0);
  if( 0==rc && !*pStmt ){
    cmpp_err_c(pp, CMPP_RC_MISUSE,
               "Empty queries are not permitted here.");
  }else if( cmpp_db_rc_v2(pp, rc, zSql, nSql) ){
    g_warn("SQLite rc=%d Offending SQL: %.*s", rc, nSql, zSql);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
sqlite3_stmt * cmpp__stmt(cmpp * pp, enum CmppStmt_e which,
                          bool prepEvenIfErr){
  if( !pp->pimpl->db.dbh && cmpp__db_init(pp) ) return NULL;
  sqlite3_stmt ** q = 0;
  char const * zSql = 0;
  switch(which){
    default:
      cmpp__fatal("Maintenance required: not a valid CmppStmt ID: %d", which);
      return NULL /* not reached */;
#define E(N,S) case CmppStmt_ ## N: zSql = S; q = &pp->pimpl->stmt.N; break;
    CmppStmt_map(E)
#undef E
  }
  assert( q );
  assert( zSql && *zSql );
  if( !*q && (!ppCode || prepEvenIfErr) ){
    cmpp__stmt_prepare(pp, false, q, zSql, -1);
  }
  return *q;
}
/* …（同文件无关代码省略）… */
static int cmpp__db_errcode(sqlite3 * const db, int sqliteCode){
  (void)db;
  int rc = 0;
  switch(sqliteCode & 0xff){
    case SQLITE_ROW:
    case SQLITE_DONE:
    case SQLITE_OK: rc = 0; break;
    case SQLITE_NOMEM: rc = CMPP_RC_OOM; break;
    case SQLITE_CORRUPT: rc = CMPP_RC_CORRUPT; break;
    case SQLITE_TOOBIG:
    case SQLITE_FULL:
    case SQLITE_RANGE: rc = CMPP_RC_RANGE; break;
    case SQLITE_NOTFOUND: rc = CMPP_RC_NOT_FOUND; break;
    case SQLITE_PERM:
    case SQLITE_AUTH:
    case SQLITE_BUSY:
    case SQLITE_LOCKED:
    case SQLITE_READONLY: rc = CMPP_RC_ACCESS; break;
    case SQLITE_CANTOPEN:
    case SQLITE_IOERR: rc = CMPP_RC_IO; break;
    case SQLITE_NOLFS: rc = CMPP_RC_UNSUPPORTED; break;
    default:
      //MARKER(("sqlite3_errcode()=0x%04x\n", rc));
      rc = CMPP_RC_DB; break;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static
int cmpp__define_impl(cmpp * const pp,
                      sqlite3_stmt * const q,
                      unsigned char const * zKey,
                      cmpp_strlen_t nKey,
                      unsigned char const *zVal,
                      cmpp_strlen_t nVal,
                      int tType,
                      bool resetStmt){
  if( 0==ppCode){
    assert( q );
    nKey = cmpp__strlenu(zKey, nKey);
    nVal = cmpp__strlenu(zVal, nVal);
    if( 0==cmpp_stmt_bind_textn(pp, q, 2, zKey, (int)nKey)
        && 0==cmpp_stmt_bind_int(pp, q, 1, tType) ){
      //g_stderr("zKey=%s\nzVal=%s\nzEq=%s\n", zKey, zVal, zEq);
      /* TODO? if tType==cmpp_TT_Blob, bind it as a blob */
      if( zVal ){
        if( nVal ){
          cmpp_stmt_bind_textn(pp, q, 3, zVal, (int)nVal);
        }else{
          /* Arguable */
          cmpp_stmt_bind_null(pp, q, 3);
        }
      }else{
        cmpp_stmt_bind_int(pp, q, 3, 1);
      }
      cmpp_stmt_step(pp, q, resetStmt);
      g_debug(pp,2,("define: %s [%s]=[%.*s]\n",
                    cmpp_tt_cstr(tType), zKey, (int)nVal, zVal));
    }
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
int cmpp__define2(cmpp *pp,
                  unsigned char const * zKey,
                  cmpp_strlen_t nKey,
                  unsigned char const *zVal,
                  cmpp_strlen_t nVal,
                  cmpp_tt_e tType){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defIns, false);
  if( q ){
    cmpp__define_impl(pp, q, zKey, nKey, zVal, nVal, tType, true);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static int cmpp__db_sq3TraceV2(unsigned dx,void*c,void*p,void*x){
  switch(dx){
    case SQLITE_TRACE_STMT:{
      char const * const zSql = x;
      cmpp * const pp = c;
      cmpp__pi(pp);
      if(pi->sqlTrace.out.out){
        char * const zExp = pi->sqlTrace.expandSql
          ? sqlite3_expanded_sql((sqlite3_stmt*)p)
          : 0;
        sqlite3_str * const s = sqlite3_str_new(pi->db.dbh);
        if( pi->dx ){
          cmpp__dx_append_script_info(pi->dx, s);
          sqlite3_str_appendchar(s, 1, ':');
          sqlite3_str_appendchar(s, 1, ' ');
        }
        sqlite3_str_appendall(s, zExp ? zExp : zSql);
        sqlite3_str_appendchar(s, 1, '\n');
        int const n = sqlite3_str_length(s);
        if( n ){
          char * const z = sqlite3_str_finish(s);
          if( z ){
            cmpp__out2(pp, &pi->sqlTrace.out, z, (cmpp_size_t)n);
            sqlite3_free(z);
          }
        }
        sqlite3_free(zExp);
      }
      break;
    }
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static void cmpp__udf_file_exists(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const char *zName;
  (void)(argc);  /* Unused parameter */
  zName = (const char*)sqlite3_value_text(argv[0]);
  if( 0!=zName ){
    struct stat sb;
    sqlite3_result_int(context, stat(zName, &sb)
                       ? 0
                       : S_ISREG(sb.st_mode));
  }
}
/* …（同文件无关代码省略）… */
static void cmpp__udf_file_size(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const char *zName;
  (void)(argc);
  zName = (const char*)sqlite3_value_text(argv[0]);
  if( 0!=zName && 0!=zName[0] ){
    struct stat sb;
    if( 0==stat(zName, &sb) ){
      sqlite3_result_int64(context, (int64_t)sb.st_size);
    }
  }
}
/* …（同文件无关代码省略）… */
static void cmpp__udf_truthy(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  (void)(argc);
  assert(1==argc);
  int buul = 0;
  sqlite3_value * const sv = argv[0];
  switch( sqlite3_value_type(sv) ){
    case SQLITE_NULL:
      break;
    case SQLITE_FLOAT:
      buul = 0.0!=sqlite3_value_double(sv);
      break;
    case SQLITE_INTEGER:
      buul = 0!=sqlite3_value_int(sv);
      break;
    case SQLITE_TEXT:
    case SQLITE_BLOB:{
      int const n = sqlite3_value_bytes(sv);
      if( n>1 ) buul = 1;
      else if( 1==n ){
        const char *z =
          (const char*)sqlite3_value_text(sv);
        buul = z
          ? 0!=strcmp(z,"0")
          : 0;
      }
    }
  }
  sqlite3_result_int(context, buul);
}
/* …（同文件无关代码省略）… */
static void cmpp__udf_compare(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  (void)(argc);  /* Unused parameter */
  assert(2==argc);
  sqlite3_value * const v1 = argv[0];
  sqlite3_value * const v2 = argv[1];
  unsigned char const * const z1 = sqlite3_value_text(v1);
  unsigned char const * const z2 = sqlite3_value_text(v2);
  int const n1 = sqlite3_value_bytes(v1);
  int const n2 = sqlite3_value_bytes(v2);
  int rv;
  if( !z1 ){
    rv = z2 ? -1 : 0;
  }else if( !z2 ){
    rv = 1;
  }else{
    rv = strncmp((char const *)z1, (char const *)z2, n1>n2 ? n1 : n2);
  }
  if(0) g_stderr("udf_compare (%s,%s) = %d\n", z1, z2, rv);
  sqlite3_result_int(context, rv);
}
/* …（同文件无关代码省略）… */
int cmpp__db_init(cmpp *pp){
  cmpp__pi(pp);
  if( pi->db.dbh || ppCode ) return ppCode;
  int rc;
  char * zErr = 0;
  const char * zDrops =
    "BEGIN EXCLUSIVE;"
    "DROP TABLE IF EXISTS " CMPP__DB_MAIN_NAME ".def;"
    "DROP TABLE IF EXISTS " CMPP__DB_MAIN_NAME ".incl;"
    "DROP TABLE IF EXISTS " CMPP__DB_MAIN_NAME ".inclpath;"
    "DROP TABLE IF EXISTS " CMPP__DB_MAIN_NAME ".predef;"
    "DROP TABLE IF EXISTS " CMPP__DB_MAIN_NAME ".ttype;"
    "DROP VIEW  IF EXISTS " CMPP__DB_MAIN_NAME ".vdef;"
    "COMMIT;"
    ;
  const char * zSchema =
    "BEGIN EXCLUSIVE;"
    "CREATE TABLE " CMPP__DB_MAIN_NAME ".def("
    /* ^^^ defines */
      "t INTEGER DEFAULT NULL,"
      /*^^ type: cmpp_tt or NULL */
      "k TEXT PRIMARY KEY NOT NULL,"
      "v TEXT DEFAULT NULL"
    ") WITHOUT ROWID;"

    "CREATE TABLE " CMPP__DB_MAIN_NAME ".incl("
    /* ^^^ files currently being included */
      "file TEXT PRIMARY KEY NOT NULL,"
      "srcFile TEXT DEFAULT NULL,"
      "srcLine INTEGER DEFAULT 0"
    ") WITHOUT ROWID;"

    "CREATE TABLE " CMPP__DB_MAIN_NAME ".inclpath("
    /* ^^^ include path. We use (ORDER BY priority DESC, rowid) to
       make their priority correct. priority should only be set by the
       #include directive for its cwd entry. */
      "priority INTEGER DEFAULT 0," /* higher sorts first */
      "dir TEXT UNIQUE NOT NULL ON CONFLICT IGNORE"
    ");"

    "CREATE TABLE " CMPP__DB_MAIN_NAME ".modpath("
    /* ^^^ module path. We use ORDER BY ROWID to make their
       priority correct. */
      "dir TEXT PRIMARY KEY NOT NULL ON CONFLICT IGNORE"
    ");"

    "CREATE TABLE " CMPP__DB_MAIN_NAME ".predef("
    /* ^^^ pre-defines */
      "t INTEGER DEFAULT NULL," /* a cmpp_tt or NULL */
      "k TEXT PRIMARY KEY NOT NULL,"
      "v TEXT DEFAULT NULL"
    ") WITHOUT ROWID;"
    "INSERT INTO " CMPP__DB_MAIN_NAME ".predef (t,k,v)"
    " VALUES(NULL,'cmpp::version','" CMPP_VERSION "')"
    ";"

    /**
       sdefs - "scoped defines" or "shadow defines". The problem these
       solve is the one of supporting a __FILE__ define in cmpp input
       sources, such that it remains valid both before and after an
       #include, but has a new name in the scope of an #include. We
       can't use savepoints for that because they're a nuclear option
       affecting _all_ #defines in the #include'd file, whereas we
       normally want #defines to stick around across files.

       See cmpp_define_shadow() and cmpp_define_unshadow().
    */
    "CREATE TABLE " CMPP__DB_MAIN_NAME ".sdef("
     "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "t INTEGER DEFAULT NULL," /* a cmpp_tt or NULL */
      "k TEXT NOT NULL,"
      "v TEXT DEFAULT NULL"
    ");"

    /**
       vdef is a view consolidating the various #define stores. It's
       intended to be used for all general-purpose fetching of defines
       and it orders the results such that the library's defines
       supercede all others, then scoped keys, then client-level
       defines.

       To push a new sdef we simply insert into sdef. Then vdef will
       order the newest sdef before any entry from the def table.
    */
    "CREATE VIEW " CMPP__DB_MAIN_NAME ".vdef(source,t,k,v) AS"
    " SELECT NULL,t,k,v FROM " CMPP__DB_MAIN_NAME ".predef"
    /* ------^^^^ sorts before numbers */
    " UNION ALL"
    " SELECT -rowid,t,k,v FROM " CMPP__DB_MAIN_NAME ".sdef"
    /* ^^^^ sorts newest of matching keys first */
    " UNION ALL"
    " SELECT 0,t,k,v FROM " CMPP__DB_MAIN_NAME ".def"
    " ORDER BY 1, 3"
    ";"


#if 0
    "CREATE TABLE " CMPP__DB_MAIN_NAME ".ttype("
    /* ^^^ token types */
      "t INTEGER PRIMARY KEY NOT NULL,"
      /*^^ type: cmpp_tt */
      "n TEXT NOT NULL,"
      /*^^ cmpp_tt_... name. */
      "s TEXT DEFAULT NULL"
      /* Symbolic or directive name, if any. */
    ");"
#endif

    "COMMIT;"
    "BEGIN EXCLUSIVE;"
    ;
  cmpp__err_reuse(pp);
  int openFlags = SQLITE_OPEN_READWRITE;
  if( pi->db.zName ){
    openFlags |= SQLITE_OPEN_CREATE;
  }
  rc = sqlite3_open_v2(
    pi->db.zName ? pi->db.zName : ":memory:",
    &pi->db.dbh, openFlags, 0);
  if(rc){
    cmpp_db_rc_v2(pp, rc, pi->db.zName
                  ? pi->db.zName
                  : ":memory:", -1);
    sqlite3_close(pi->db.dbh);
    pi->db.dbh = 0;
    assert(ppCode);
    return rc;
  }
  sqlite3_busy_timeout(pi->db.dbh, 5000);
  sqlite3_db_config(pi->db.dbh, SQLITE_DBCONFIG_MAINDBNAME,
                    CMPP__DB_MAIN_NAME);
  rc = sqlite3_trace_v2(pi->db.dbh, SQLITE_TRACE_STMT,
                        cmpp__db_sq3TraceV2, pp);
  if( cmpp_db_rc(pp, rc, "Installing tracer failed") ){
    goto end;
  }
  //g_warn("Schema:\n%s\n",zSchema);
  struct {
    /* SQL UDFs */
    char const * const zName;
    void (*xUdf)(sqlite3_context *,int,sqlite3_value **);
    int arity;
    int flags;
  } aFunc[] = {
    {
      .zName = "cmpp_file_exists",
      .xUdf  = cmpp__udf_file_exists,
      .arity = 1,
      .flags = SQLITE_UTF8 | SQLITE_DIRECTONLY
    },
    {
      .zName = "cmpp_file_size",
      .xUdf  = cmpp__udf_file_size,
      .arity = 1,
      .flags = SQLITE_UTF8 | SQLITE_DIRECTONLY
    },
#if 0 /* See cmpp__udf_truthy() */
    {
      .zName = "cmpp_truthy",
      .xUdf  = cmpp__udf_truthy,
      .arity = 1,
      .flags = SQLITE_UTF8 | SQLITE_DIRECTONLY | SQLITE_DETERMINISTIC
    },
    {
      .zName = "cmpp_compare",
      .xUdf  = cmpp__udf_compare,
      .arity = 2,
      .flags = SQLITE_UTF8 | SQLITE_DIRECTONLY | SQLITE_DETERMINISTIC
    }
#endif
  };
  assert( 0==rc );
  for( unsigned int i = 0; 0==rc && i < sizeof(aFunc)/sizeof(aFunc[0]); ++i ){
    rc = sqlite3_create_function(
      pi->db.dbh, aFunc[i].zName, aFunc[i].arity,
      aFunc[i].flags, 0, aFunc[i].xUdf, 0, 0
    );
  }
  if( cmpp_db_rc(pp, rc, "UDF registration failed.") ){
    return ppCode;
  }
  if( pi->db.zName ){
    /* Drop all cmpp tables when using a persistent db so that we are
       not beholden to a given structure.  TODO: a config flag to
       toggle this. */
    rc = sqlite3_exec(pi->db.dbh, zDrops, 0, 0, &zErr);
  }
  if( !rc ){
    rc = sqlite3_exec(pi->db.dbh, zSchema, 0, 0, &zErr);
  }

  if( !rc ){
    extern int sqlite3_series_init(sqlite3 *, char **, const sqlite3_api_routines *);
    rc = sqlite3_series_init(pi->db.dbh, &zErr, NULL);
  }

  if(rc){
    if( zErr ){
      cmpp_errf(pp, cmpp__db_errcode(pi->db.dbh, rc),
                "SQLite error #%d initializing DB: %s", rc, zErr);
      sqlite3_free(zErr);
    }else{
      cmpp_errf(pp, cmpp__db_errcode(pi->db.dbh, rc),
                "SQLite error #%d initializing DB", rc);
    }
    goto end;
  }

#if 0
  while(0){
    /* Insert the ttype mappings. We don't yet make use of this but
       only for lack of a use case ;). */
    sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_insTtype, false);
    if( !q ) goto end;
#define E(N,STR)                                                        \
    cmpp_stmt_bind_int(pp, q, 1, cmpp_tt_ ## N);                            \
    cmpp_stmt_bind_textn(pp, q, 2,                                          \
                     ustr_c("cmpp_tt_ " # N), sizeof("cmpp_tt_" # N)-1); \
    if( STR ) cmpp_stmt_bind_textn(pp, q, 3, ustr_c(STR), sizeof(STR)-1);   \
    else cmpp_stmt_bind_null(pp, q, 3);                                     \
    if( SQLITE_DONE!=cmpp_stmt_step(pp, q, true) ) return ppCode;
    cmpp_tt_map(E)
#undef E
    sqlite3_finalize(q);
    pi->stmt.insTtype = 0;
    break;
  }
#endif

end:
  if( !ppCode ){
    /*
    ** Keep us from getting in the situation later that delayed
    ** preparation if one of the savepoint statements fails (e.g. due
    ** to OOM or memory corruption). We need to be able to run these
    ** even in OOM cases (while admitting that they may fail to run in
    ** OOM cases).
    */
    cmpp__stmt(pp, CmppStmt_spBegin, false);
    cmpp__stmt(pp, CmppStmt_spRelease, false);
    cmpp__stmt(pp, CmppStmt_spRollback, false);
    cmpp__lazy_init(pp);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static void cmpp_f_noop(cmpp_f_args const * args){
  (void)args;
}
/* …（同文件无关代码省略）… */
static int cmpp_kav_each_f_define__group(
  cmpp *pp,
  unsigned char const *zKey, cmpp_size_t nKey,
  unsigned char const *zVal, cmpp_size_t nVal,
  void* callbackState
){
  if( (callbackState==pp)
      && cmpp__has(pp, (char const*)zKey, nKey) ){
    return ppCode;
  }
  return cmpp__define2(pp, zKey, nKey, zVal, nVal,
                       cmpp_tt_DbString);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_define(cmpp_f_args const * args){
  assert(args->argc>0 && "V4 arg semantics.");
  assert(args->argv && "V4|!RAW arg semantics.");
  assert( cmpp_arg_equals(args->argv[0],
                          (char const*)args->dx->d->name.z) );
  cmpp_dx * const dx = args->dx;
  if( args->argc<2 ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                "Expecting one or more arguments");
    return;
  }
  cmpp_arg const * aKey = 0;
  int nChomp = 0;
  unsigned nHeredoc = 0;
  unsigned char acHeredoc[128] = {0} /* TODO: cmpp_args_clone() */;
  bool ifNotDefined = false /* true if '?' arg */;
  cmpp_arg const *aAppend = 0;
  bool raw = false;
#define checkIsDefined(ARG) \
    if(ifNotDefined && (cmpp__has(dx->pp, (char const*)ARG->z, ARG->n) \
                        || dxppCode)) break
  for( unsigned i = 1; 0==dxppCode && i < args->argc; ++i){
    cmpp_arg const * arg = args->argv[i];
    //g_warn("arg=%s", arg->z);
    if( 1==i && cmpp_arg_equals_c(arg, "?") ){
      /* Only set the key if it's not already defined. */
      ifNotDefined = true;
      continue;
    }
    switch( arg->ttype ){
      case cmpp_tt_OpShiftL3:
        ++nChomp;
        /* fall through */
      case cmpp_tt_OpShiftL:
        if( i<1 || i+1<args->argc ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "Ill-placed '%s'.", arg->z);
        }else if( arg->n >= sizeof(acHeredoc)-1 ){
          cmpp_dx_errf(dx, CMPP_RC_RANGE,
                      "Heredoc name is too large.");
        }else if( !aKey ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "Missing key before %s.",
                          cmpp__tt_cstr2(cmpp_tt_OpShiftL, false));
        }else{
          assert( aKey );
          nHeredoc = aKey->n;
          memcpy(acHeredoc, aKey->z, aKey->n+1/*NUL*/);
        }
        break;
      case cmpp_tt_OpCmpEq:
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "Ill-placed '%s'.", arg->z);
          break;
      case cmpp_tt_IntDec:
      case cmpp_tt_StringBT:
      case cmpp_tt_StringDQ:
      case cmpp_tt_StringSQ:
      case cmpp_tt_Word:
        if( cmpp_arg_isflag_c(arg,"-chomp") ){
          ++nChomp;
          break;
        }
        if( cmpp_arg_isflag_c(arg,"-raw") ){
          raw = true;
          break;
        }
        if( cmpp_arg_isflag_c(arg,"-append")
            || cmpp_arg_isflag_c(arg,"-a") ){
          aAppend = args->argv[++i];
          if( !aAppend ){
            cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                            "Expecting argument for %s",
                            arg->z);
          }
          arg = aAppend;
          break;
        }
      handle_value:
        if( aKey ){
          /* This is the second arg - the value */
#if 1
          if( i+1<args->argc ){
            cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                            "Unexpected token after %s",
                            arg->z);
            break;
          }
#endif
          checkIsDefined(aKey);
          cmpp_b * const os = cmpp_b_borrow(dx->pp);
          cmpp_b * const ba = aAppend ? cmpp_b_borrow(dx->pp) : 0;
          while( os ){
            if( ba ){
              cmpp__get_b(dx->pp, aKey->z, aKey->n, ba, false);
              if( dxppCode ) break;
              if( 0 ){
                g_warn("key=%s\n", aKey->z);
                g_warn("ba=%u %.*s\n", ba->n, ba->n, ba->z);
              }
            }
            if( cmpp_arg_interpolate(args->pp, arg, os,
                              cmpp_arg_interpolate_BRACE_CALL) ) break;
            cmpp_b * const which = (ba && ba->n) ? ba : os;
            if( which==ba && os->n ){
              if( ba->n ) cmpp_b_append4(dx->pp, ba, aAppend->z, aAppend->n);
              cmpp_b_append4(args->pp, ba, os->z, os->n);
            }
            int n = nChomp;
            while( n-- && cmpp_b_chomp(which) ){}
            cmpp__define2(dx->pp, aKey->z, aKey->n, which->z, which->n,
                          arg->ttype);
            if( 0 ){
              g_warn("aKey=%u z=[%.*s]\n", aKey->n, (int)aKey->n, aKey->z);
              g_warn("nExp=%u z=[%.*s]\n", which->n, (int)which->n, which->z);
            }
            break;
          }
          cmpp_b_return(dx->pp, os);
          cmpp_b_return(dx->pp, ba);
          aKey = 0;
        }else if( cmpp_tt_Word!=arg->ttype ){
          cmpp_dx_errf(dx, CMPP_RC_TYPE,
                      "Expecting a define-name token here.");
        }else if( i+1<args->argc ){
          aKey = arg;
        }else{
          /* No value = a value of 1. */
          checkIsDefined(arg);
          cmpp__define2(dx->pp, arg->z, arg->n,
                        ustr_c("1"), 1, cmpp_tt_IntDec);
        }
        break;
      case cmpp_tt_GroupSquiggly:
        assert( !acHeredoc[0] );
        if( aKey ){
          /* Treat this as a value */
          goto handle_value;
        }else if( (ifNotDefined ? i>2 : i>1) || i+1<args->argc ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "{...} must be the only argument.")
            /* This is for simplicity's sake. */;
        }else{
          cmpp_kav_each(args->pp, arg->z, arg->n,
                        cmpp_kav_each_f_define__group,
                        ifNotDefined ? args->pp : NULL,
                        cmpp_kav_each_F_NOT_EMPTY
                        | cmpp_kav_each_F_CALL_VAL
                        | cmpp_kav_each_F_PARENS_EXPR
                        //TODO cmpp_kav_each_F_IF_UNDEF
          );
        }
        aKey = 0;
        break;
      case cmpp_tt_GroupParen:{
        if( !aKey ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "(...) is not permitted as a key.");
          break;
        }
        checkIsDefined(aKey);
        int64_t d = 0;
        if( 0==cmpp__expr_eval_paren(args->pp, arg, &d) ){
          char exprBuf[32] = {0};
          cmpp_size_t nVal =
            (cmpp_size_t)snprintf(&exprBuf[0],
                                  sizeof(exprBuf), "%" PRIi64, d);
          assert(nVal>0);
          cmpp__define2(dx->pp, aKey->z, aKey->n,
                        ustr_c(&exprBuf[0]), nVal,
                        cmpp_tt_IntDec);
        }
        break;
      }
      case cmpp_tt_GroupBrace:{
        if( !aKey ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "[...] is not permitted as a key.");
          break;
        }
        checkIsDefined(aKey);
        cmpp_b * const b = cmpp_b_borrow(dx->pp);
        if( b && 0==cmpp_call_str(dx->pp, arg->z, arg->n, b, 0) ){
          int n = nChomp;
          while( n-- && cmpp_b_chomp(b) ){}
          cmpp__define2(dx->pp, aKey->z, aKey->n,
                        b->z, b->n, cmpp_tt_AnyType);
        }
        cmpp_b_return(dx->pp, b);
        break;
      }
      default:
        // TODO: treat (...) as an expression
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "Unhandled arg type %s: %s",
                        cmpp__tt_cstr2(arg->ttype, true), arg->z);
        break;
    }
  }
  if( 0==dxppCode && nHeredoc ){
    // Process (#define KEY <<)
    cmpp_b * const os = cmpp_b_borrow(dx->pp);
    assert( args->d->closer );
    if( os &&
        0==cmpp_dx_consume_b(dx, os, &args->d->closer, 1,
                             raw
                             ? cmpp_dx_consume_F_ATPOL_OFF
                             : cmpp_dx_consume_F_PROCESS_OTHER_D) ){
      while( nChomp-- && cmpp_b_chomp(os) ){}
      g_debug(dx->pp,2,("define  heredoc: [%s]=[%.*s]\n",
                       acHeredoc, (int)os->n, os->z));
      if( !ifNotDefined
          || !cmpp__has(dx->pp, (char const*)acHeredoc, nHeredoc) ){
        cmpp__define2(
          dx->pp, acHeredoc, nHeredoc, os->z, os->n,
          cmpp_tt_DbString
        );
      }
    }
    cmpp_b_return(dx->pp, os);
  }
#undef checkIsDefined
  return;
}
/* …（同文件无关代码省略）… */
static void cmpp_f_undef(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  if( args->argc<2 ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                "Expecting one or more arguments");
    return;
  }
  cmpp_size_t n = 0;
  char const * z;
  for( unsigned i = 1; i < args->argc && 0==dxppCode; ++i ){
    cmpp_arg const * arg = args->argv[i];
    z = cmpp_arg_cstr2(arg, &n);
    if( 0 ){
      g_stderr("  %s: %s %p n=%d %.*s\n", args->d->name.z,
               cmpp__tt_cstr2(arg->ttype, true), arg->z,
               (int)arg->n, (int)arg->n, arg->z);
    }
    if( cmpp_tt_Word==arg->ttype ){
#if 0
      /* Too strict? */
      if( 0==cmpp__legal_key_check(dx->pp, z, n, false) ) {
        cmpp_undefine(dx->pp, (char const *)z);
      }
#else
      cmpp_undefine(dx->pp, z, n);
#endif
    }else{
      cmpp_errf(dx->pp, CMPP_RC_MISUSE, "Invalid arg for %s: %s",
                args->d->name.z, arg->z);
    }
  }
}

/* Impl. for #eval. */
/* …（同文件无关代码省略）… */
static void cmpp_f_expand(cmpp_f_args const * args){
  if( args->argc<2 || args->argc>3 ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE,
              "%sexpand requires one or two arguments",
              cmpp__dx_zdelim(args->dx));
    return;
  }
  cmpp_arg const * argKav = 0 /* {key -> value ...} */;
  cmpp_arg const * argToB = 0 /* one of X or Y from: #expand X Y */;
  cmpp_arg const * aLhs = 0;
  cmpp_arg const * aRhs = 0;
  cmpp_arg const * aErr = 0;  /* Current arg for error reportin */
  cmpp_b * const osIn = cmpp_b_borrow(args->pp);
  int nChomp = 0    /* Strip nChomp newlines from result */;
  int unChomp = 0   /* Add unChomp newlines to result */;
  bool raw = false  /* interpret block content as raw or
                       preprocessor-ready? */;
  bool inSp = false /* started a local savepoint? */;
  if( !osIn ) goto end;
  for( unsigned i = 1; i < args->argc; ++i ){
    cmpp_arg const * arg = args->argv[i];
    aErr = arg;
    switch( arg->ttype ){
      case cmpp_tt_Word:
        if( cmpp_arg_isflag_c(arg,"-chomp") ){
          ++nChomp;
          continue;
        }
        if( cmpp_arg_isflag_c(arg,"-unchomp") ){
          ++unChomp;
          continue;
        }
        if( cmpp_arg_isflag_c(arg,"-raw") ){
          raw = true;
          continue;
        }
        if( !aLhs ){
          aLhs = arg;
          continue;
        }
        goto unexpected_arg;
      default:
        assert( !aRhs );
        if( aLhs ){
          if( i+1<args->argc ){
            assert( arg!=aLhs );
            assert( !aRhs );
            goto unexpected_arg;
          }
          aRhs = arg;
        }else{
          aLhs = arg;
        }
        continue;
    }
  }
  if( !aLhs ){
    cmpp_err(args->pp, CMPP_RC_MISUSE,
             "Expecting one or two arguments.", -1);
    goto end;
  }
  if( aRhs ){
    if( cmpp_tt_GroupSquiggly!=aRhs->ttype ){
      aErr = aRhs;
      goto unexpected_arg;
    }
    argKav = aRhs;
    argToB = aLhs;
  }else{
    if( cmpp_tt_GroupSquiggly==aLhs->ttype ){
      argKav = aLhs;
    }else{
      argToB = aLhs;
    }
  }
  assert( argKav || argToB );
  if( argToB ){
    if( cmpp_arg_interpolate(args->pp, argToB, osIn,
                      cmpp_arg_interpolate_BRACE_CALL) ){
      goto end;
    }
  }
  if( argKav ){
    assert(!argKav->next);
    if( (aErr = argKav->next) ){
      goto unexpected_arg;
    }
    if( cmpp_sp_begin(args->pp) ) goto end;
    inSp = true;
    if( cmpp_kav_each(args->pp, argKav->z, argKav->n,
                      cmpp_kav_each_f_define__group,
                      0,
                      cmpp_kav_each_F_EXPAND_VAL
                      | cmpp_kav_each_F_CALL_VAL
                      | cmpp_kav_each_F_PARENS_EXPR) ){
      goto end;
    }
  }
  if( argToB && cmpp_tt_GroupSquiggly==argToB->ttype ){
    cmpp_outputer xout = cmpp_outputer_b;
    cmpp_outputer old = {0};
    cmpp_b * const b = cmpp_b_borrow(args->pp);
    if( !(xout.state = b) ) goto end;
    cmpp__outputer_swap(args->pp, &xout, &old);
    cmpp_process_string(args->pp, "#expand buffer", osIn->z, osIn->n);
    cmpp__outputer_swap(args->pp, &old, &xout);
    cmpp_b_swap(b, osIn);
    cmpp_b_return(args->pp, b);
    if( cmpp_err_has(args->pp) ) goto end;
  }
  if( !argToB ){
    assert(!osIn->n);
    if( args->isDxCall ){
      cmpp_errf(args->pp, CMPP_RC_MISUSE,
                "#expand.../#expand is not legal for [call].");
      goto end;
    }
    if( cmpp_dx_consume_b(args->dx, osIn, &args->d->closer, 1,
                          raw
                          ? cmpp_dx_consume_F_ATPOL_OFF
                          : cmpp_dx_consume_F_PROCESS_OTHER_D) ){
      goto end;
    }
  }
  while( nChomp-- && cmpp_b_chomp(osIn) ){}
  while( unChomp-- && cmpp_b_append_ch(osIn, '\n') ){}
  //MARKER(("Emitting %d bytes: %.*s\n", (int)osIn->n, (int)osIn->n, osIn->z));
  cmpp_out_expand(args->pp, NULL, osIn->z, osIn->n, cmpp_atpol_CURRENT);

end:
  if( inSp ){
    cmpp_sp_rollback(args->pp);
  }
  cmpp_b_return(args->pp, osIn);
  return;
unexpected_arg:
  assert( aErr );
  cmpp_errf(args->pp, CMPP_RC_MISUSE, "Unexpected argument: %.*s",
            (int)aErr->n, aErr->z);
  goto end;
}
/* …（同文件无关代码省略）… */
static void cmpp_f_once(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  cmpp_d const * const d = args->d;
  assert(d);
  assert(d->closer);
  if( args->argc>1 ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                    "Expecting no arguments");
    return;
  }
  cmpp_b * const b = cmpp_b_borrow(args->pp);
  if( !b ) return;
  else{
    /* Synthesize an ID from the source code position.  Typically
       we'll hit any position only once in a given run, but #include
       and #query loops can cause re-visits. */
    cmpp__delim const * const dlim = cmpp__dx_delim(args->dx);
    cmpp_b_append4(args->pp, b, dlim->open.z, dlim->open.n);
    cmpp_b_append4(args->pp, b, d->name.z, d->name.n);
    cmpp_b_append_ch(b, ':');
    char const * zFile = 0;
    cmpp_size_t lnNo = 0;
    cmpp_dx_src_pos_info(dx, &zFile, &lnNo);
    cmpp_b_append4_str(args->pp, b, zFile, -1);
    cmpp_b_append_ch(b, ':');
    cmpp_b_append_i32(b, (int)lnNo);
    if( b->errCode ) goto end;
  }
  if( 0 ){
    g_warn("#once dline=%d pos.lineNo=%d, key: %s",
           (int)dx->pimpl->dline.lineNo,
           (int)dx->pimpl->pos.lineNo,
           b->z);
  }
  int const had = cmpp__has(args->pp, (char const *)b->z, b->n);
  if( dxppCode ) goto end;
  else if( had ){
    /* Skip until #/once. */
    cmpp__dx_skip(dx, &d->closer, 1);
  }else if( 0==cmpp_define_v2(args->pp, (char const*)b->z, "1") ){
    cmpp_dx_consume(dx, NULL, &d->closer, 1,
                    cmpp_dx_consume_F_PROCESS_OTHER_D);
  }
end:
  cmpp_b_return(args->pp, b);
  return;
}
/* …（同文件无关代码省略）… */
static int cmpp__including_has(cmpp *pp, unsigned const char * zName){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclHas, false);
  if( q && 0==cmpp_stmt_bind_text(pp, q, 1, zName) ){
    if(SQLITE_ROW == cmpp_stmt_step(pp, q, true)){
      rc = 1;
    }else{
      rc = 0;
    }
    g_debug(pp,2,("inclpath has [%s] = %d\n",zName, rc));
  }
  return rc;
}
/* …（同文件无关代码省略）… */
char * cmpp__include_search(cmpp *pp, unsigned const char * zKey,
                            cmpp_size_t * nVal){
  char * zName = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclSearch, false);
  if( nVal ) *nVal = 0;
  if( q && 0==cmpp_stmt_bind_text(pp, q, 1, zKey) ){
    int const rc = cmpp_stmt_step(pp, q, false);
    if(SQLITE_ROW==rc){
      const unsigned char * z = sqlite3_column_text(q, 0);
      int const n = sqlite3_column_bytes(q,0);
      zName = n ? sqlite3_mprintf("%.*s", n, z) : 0;
      if( n ) cmpp_check_oom(pp, zName);
      if( nVal ) *nVal = n;
    }
    cmpp_stmt_reset(pp, q);
  }
  return zName;
}
/* …（同文件无关代码省略）… */
static int cmpp__include_rm(cmpp *pp, unsigned const char * zKey){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclDel, false);
  if( q ){
    cmpp_stmt_bind_text(pp, q, 1, ustr_c(zKey));
    cmpp_stmt_step(pp, q, true);
    g_debug(pp,2,("incl rm [%s]\n", zKey));
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static int cmpp__including_add(cmpp *pp, unsigned const char * zKey,
                               unsigned const char * zSrc, cmpp_size_t srcLine){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclIns, false);
  if( q ){
    cmpp_stmt_bind_text(pp, q, 1, zKey);
    cmpp_stmt_bind_text(pp, q, 2, zSrc);
    cmpp_stmt_bind_int(pp, q, 3, srcLine);
    cmpp_stmt_step(pp, q, true);
    g_debug(pp,2,("is-including-file add [%s] from [%s]:%"
                  CMPP_SIZE_T_PFMT "\n", zKey, zSrc, srcLine));
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static void cmpp_f_include(cmpp_f_args const * args){
  char * zResolved = 0;
  cmpp_size_t nResolved;
  cmpp_b * const ob = cmpp_b_borrow(args->pp);
  bool raw = false;
  bool resolve = false;
  cmpp_arg const * argDelim = 0;
  if( !ob ){
    goto end;
  }
  unsigned i = 1;
  for( ; i < args->argc; ++i){
    cmpp_arg const * arg = args->argv[i];
    if( cmpp_arg_isflag_c(arg, "-raw") ){
      if( resolve ) goto no_raw_resolve;
      raw = true;
      continue;
    }
    if( cmpp_arg_isflag_c(arg, "-delimiter") ){
      if( ++i==args->argc ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "-delimiter requires an argument");
        goto end;
      }
      argDelim = args->argv[i];
      continue;
    }
    if( cmpp_arg_isflag_c(arg, "-resolve") ){
      if( raw ){
      no_raw_resolve:
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "-raw and -resolve may not be used together.");
        goto end;
      }else{
        resolve = true;
        continue;
      }
    }
    break;
#undef FLAG
  }
  if( i>=args->argc ){
    cmpp_errf(args->pp, CMPP_RC_SYNTAX,
              "Expecting at least one filename argument.");
  }
  if( argDelim && (raw || resolve) ) argDelim = 0;
  if( argDelim ){
    if( cmpp_arg_interpolate(args->pp, argDelim, cmpp_b_reuse(ob),
                      cmpp_arg_interpolate_BRACE_CALL)
        || cmpp_delimiter_push(args->pp, (char*)ob->z) ){
      argDelim = 0/*our signal to not pop the delimiter*/;
      goto end;
    }
  }
  for( ; i < args->argc; ++i ){
    cmpp_arg const * arg = args->argv[i];
    if( cmpp_arg_interpolate(args->pp, arg, cmpp_b_reuse(ob),
                      cmpp_arg_interpolate_BRACE_CALL) ){
      break;
    }
    //g_stderr("zFile=%s zResolved=%s\n", zFile, zResolved);
    if(!raw && !resolve && cmpp__including_has(args->pp, ob->z)){
      /* Note that different spellings of the same filename
      ** will elude this check, but that seems okay, as different
      ** spellings means that we're not re-running the exact same
      ** invocation. We might want some other form of multi-include
      ** protection, rather than this, however. There may well be
      ** sensible uses for recursion. */
      cmpp_errf(args->pp, CMPP_RC_RANGE,
                "Recursive include of file: %s",
                ob->z);
      break;
    }
    cmpp_mfree(zResolved);
    nResolved = 0;
    zResolved = cmpp__include_search(args->pp, ob->z, &nResolved);
    if( !zResolved ){
      if( !cmpp_err_has(args->pp) ){
        cmpp_errf(args->pp, CMPP_RC_NOT_FOUND,
                  "file not found: %s", ob->z);
      }
      break;
    }
    if( resolve ){
      cmpp_out_raw(args->pp, zResolved, nResolved);
    }else if( raw ){
      if( !args->pp->pimpl->out.out ) break;
      FILE * const fp = cmpp_fopen(zResolved, "r");
      if( fp ){
        int const rc = cmpp_stream(cmpp_input_f_FILE, fp,
                                   args->pp->pimpl->out.out,
                                   args->pp->pimpl->out.state);
        if( rc ){
          cmpp_errf(args->pp, rc, "Unknown error streaming file %s.",
                    arg->z);
        }
        cmpp_fclose(fp);
      }else{
        cmpp_errf(args->pp, cmpp_errno_rc(errno, CMPP_RC_IO),
                  "Unknown error opening file %s.", arg->z);
      }
    }else{
      cmpp__including_add(args->pp, ob->z,
                          ustr_c(args->dx->sourceName),
                          args->dx->pimpl->dline.lineNo);
      cmpp_process_file(args->pp, zResolved);
      cmpp__include_rm(args->pp, ob->z);
    }
  }
end:
  if( argDelim ){
    cmpp_delimiter_pop(args->pp);
  }
  cmpp_mfree(zResolved);
  cmpp_b_return(args->pp, ob);
}
/* …（同文件无关代码省略）… */
static void cmpp__dx_args_after_name(cmpp_dx const * dx,
                                     cmpp_d const * d,
                                     unsigned char const **pz,
                                     cmpp_size_t *pn){
  unsigned char const * z = dx->args.z;
  unsigned char const * const zEnd = z + dx->args.nz;
  assert( dx->args.nz >= d->name.n );
  assert( 0==memcmp(z, d->name.z, d->name.n) );
  z += d->name.n;
  cmpp_skip_snl(&z, zEnd);
  *pz = z;
  *pn = (cmpp_size_t)(zEnd - z);
}
/* …（同文件无关代码省略）… */
static void cmpp__dump_sizeofs(void){
#define SO(X) printf("sizeof(" # X ") = %u\n", (unsigned)sizeof(X))
  SO(cmpp);
  SO(cmpp_api_thunk);
  SO(cmpp_arg);
  SO(cmpp_args);
  SO(cmpp_args_pimpl);
  SO(cmpp_b);
  SO(cmpp_d);
  SO(cmpp_d_reg);
  SO(cmpp__delim);
  SO(cmpp__delim_list);
  SO(cmpp_dx);
  SO(cmpp__dx_pimpl);
  SO(cmpp__dx_pimpl);
  SO(cmpp_outputer);
  SO(cmpp__pimpl);
  SO(((cmpp__pimpl*)0)->stmt);
  SO(((cmpp__pimpl*)0)->policy);
  SO(cmpp_tizer);
  SO(cmpp_token);
  SO(CmppArgList);
  SO(CmppDLine);
  SO(CmppDList);
  SO(CmppDList_entry);
  SO(PodList__atpol);
  printf("cmpp_tt__last   = %d\n",
         cmpp_tt__last);
#undef SO
}
/* …（同文件无关代码省略）… */
static void cmpp_f_pragma(cmpp_f_args const * args){
  if(args->argc<2){
    cmpp_errf(args->pp, CMPP_RC_SYNTAX, "Expecting an argument");
    return;
  }else if(args->argc>2){
    cmpp_errf(args->pp, CMPP_RC_SYNTAX, "Too many arguments");
    return;
  }
  cmpp_arg const * arg = args->argv[1];
  const char * const zArg = (char const *)arg->z;
#define M(X) 0==strcmp(zArg,X)
  if(M("defines")){
    cmpp__dump_defines(args->pp, stderr, 1);
    return;
  }
  if(M("sizeof")){
    cmpp__dump_sizeofs();
    return;
  }
  if(M("chomp-F")){
    args->pp->pimpl->flags.chompF = 1;
    return;
  }
  if(M("no-chomp-F")){
    args->pp->pimpl->flags.chompF = 0;
    return;
  }
  if(M("api-thunk")){
    return;
  }/*API thunk*/

#undef M
  cmpp_errf(args->pp, CMPP_RC_NOT_FOUND, "Unknown pragma: %s", zArg);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_savepoint(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  if( 2 != args->argc ){
    cmpp_dx_errf(dx, CMPP_RC_SYNTAX, "Expecting one argument");
    return;
  }
  cmpp_arg const * const arg = args->argv[1];
  if( cmpp_arg_isflag_c(arg, "begin") ){
    cmpp__dx_sp_begin(dx);
  }else if( cmpp_arg_isflag_c(arg, "rollback") ){
    cmpp__dx_sp_rollback(dx);
  }else if( cmpp_arg_isflag_c(arg, "commit") ){
    cmpp__dx_sp_commit(dx);
  }else{
    cmpp_errf(args->pp, CMPP_RC_MISUSE,
              "Unknown savepoint option: %s",
              arg->z);
  }
}
/* …（同文件无关代码省略）… */
static void cmpp_f_stderr(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  if(dx->args.z){
    g_stderr("%s:%" CMPP_SIZE_T_PFMT ": %.*s\n", dx->sourceName,
             dx->pimpl->dline.lineNo,
             (int)dx->args.nz, dx->args.z);
  }else{
    cmpp_d const * d = args->d;
    g_stderr("%s:%" CMPP_SIZE_T_PFMT ": (no %s%s argument)\n",
             dx->sourceName, dx->pimpl->dline.lineNo,
             cmpp_dx_delim(dx), d->name.z);
  }
}
/* …（同文件无关代码省略）… */
static void cmpp_f_at(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  if( args->argc<2 ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                    "Expecting arguments.");
    return;
  }
  enum ops { op_none, op_set, op_push, op_pop, op_heredoc };
  enum popWhichE { pop_policy = 0x01, pop_delim = 0x02,
                   pop_both = pop_policy | pop_delim };
  enum ops op = op_none          /* what to do */;
  int popWhich = 0               /* what to pop */;
  bool gotPolicy = false;
  bool checkedCallForm = !args->isDxCall;
  cmpp_arg const * argDelimO = 0 /* @token@ opener */;
  cmpp_arg const * argDelimC = 0 /* @token@ closer */;
  cmpp__pi(args->pp);
  cmpp_atpol_e polNew = cmpp_atpol_get(args->pp);
  for( unsigned i = 1; i < args->argc; ++i ){
    cmpp_arg const * arg = args->argv[i];
    //g_warn("arg=%s", arg->z);
    if( !checkedCallForm ){
      assert( args->isDxCall );
      checkedCallForm = true;
      if( cmpp_arg_equals_c(arg, "policy") ){
        char const * z =
          cmpp__atpol_name(args->pp, cmpp__policy(args->pp,at));
        if( z ){
          cmpp_out_raw(args->pp, z, strlen(z));
        }
      }else if( cmpp_arg_equals_c(arg, "delimiter") ){
        char const * zO = 0;
        char const * zC = 0;
        cmpp_atdelim_get(args->pp, &zO, &zC);
        if( zC ){
          cmpp_out_raw(args->pp, zO, strlen(zO));
          cmpp_out_raw(args->pp, " ", 1);
          cmpp_out_raw(args->pp, zC, strlen(zC));
        }
        goto end;
      }else{
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                     "In call form, '%s' expects one of "
                     "'policy' or delimiter'.",
                     args->d->name.z);
      }
      goto end;
    }/* checkedCallForm */
    if( !argDelimC && op_none==op ){
      /* Look for push|pop. */
      if( cmpp_arg_equals_c(arg, "pop") ){
        if( ++i==args->argc ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "'pop' expects arguments of 'policy' "
                          "and/or 'delimiter' and/or 'both'.");
          goto end;
        }
        for( ; i < args->argc; ++i ){
          arg = args->argv[i];
          if( 0==(pop_policy & popWhich)
              && cmpp_arg_equals_c(arg, "policy") ){
            popWhich |= pop_policy;
          }else if( 0==(pop_delim & popWhich)
                    && cmpp_arg_equals_c(arg, "delimiter") ){
            popWhich |= pop_delim;
          }else if( 0==(pop_both & popWhich)
                    && cmpp_arg_equals_c(arg, "both") ){
            popWhich |= pop_both;
          }else{
            cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                         "Invalid argument to 'pop': %s",
                         arg->z);
            goto end;
          }
        }
        op = op_pop;
        break;
      }/* pop */
      if( cmpp_arg_equals_c(arg, "push") ){
        op = op_push;
        continue;
      }
      if( cmpp_arg_equals_c(arg, "set") ){
        /* set is implied if neither of push/pop are and we get
           a policy name. */
        op = op_set;
        continue;
      }
      /* Fall through */
    }/* !argDelimC && op_none==op */
    if( !gotPolicy && cmpp_arg_equals_c(arg, "policy") ){
      if( ++i==args->argc ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "'policy' requires a policy name argument.");
        goto end;
      }
      arg = args->argv[i];
      polNew = cmpp_atpol_from_str(NULL, (char const*)arg->z);
      if( cmpp_atpol_invalid==polNew ){
        cmpp_atpol_from_str(args->pp, (char const*)arg->z)
          /* Will set the error state to something informative. */;
        goto end;
      }
      if( op_none==op ) op = op_set;
      gotPolicy = true;
      continue;
    }
    if( !argDelimC && cmpp_arg_equals_c(arg, "delimiter") ){
      assert( !argDelimO && !argDelimC );
      if( i+2>=args->argc ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "'delimiter' requires two arguments.");
        goto end;
      }
      argDelimO = args->argv[++i];
      argDelimC = args->argv[++i];
      if( op_none==op ) op = op_set;
      continue;
    }
    if( op_pop!=op ){
      if( cmpp_arg_equals_c(arg,"<<") ){
        if( i+1<args->argc ) {
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "'%s' must be the final argument.",
                          arg->z);
          goto end;
        }
        op = op_heredoc;
        break;
      }
    }
    cmpp_dx_errf(dx, CMPP_RC_MISUSE, "Unhandled argument: %s", arg->z);
    return;
  }/*arg collection*/

  assert( !dxppCode );
  assert( cmpp_atpol_invalid!=polNew );

#define popcheck(LIST)     \
    if(dxppCode) goto end; \
    if(!LIST.n) goto bad_pop

  if( op_pop==op ){
    assert( popWhich>0 && popWhich<=3 );
    if( pop_policy & popWhich ){
      popcheck(pi->policy.at);
      cmpp_atpol_pop(args->pp);
    }
    if( pop_delim & popWhich ){
      popcheck(pi->delim.at);
      cmpp_atdelim_pop(args->pp);
    }
    goto end;
  }

  assert( op_set==op || op_push==op || op_heredoc==op );
  if( argDelimC ){
    /* Push or set the @token@ delimiters */
    if( 0 ){
      g_warn("%s @delims@: %s %s", (op_set==op) ? "set" : "push",
             argDelimO->z, argDelimC->z);
    }
    if( op_push==op || op_heredoc==op ){
      if( cmpp_atdelim_push(args->pp, (char const*)argDelimO->z,
                            (char const*)argDelimC->z) ){
        goto end;
      }
      argDelimO = 0 /* Re-use argDelimC as a flag in case we need to
                       roll this back on an error below. */;
    }else{
      assert( op_set==op );
      if( cmpp_atdelim_set(args->pp, (char const*)argDelimO->z,
                           (char const*)argDelimC->z) ){
        goto end;
      }
      argDelimO = argDelimC = 0;
    }
  }

  assert( !dxppCode );
  assert( !argDelimO );
  if( op_heredoc==op ){
    if( cmpp_atpol_push(args->pp, polNew) ){
      if( argDelimC ){
        popcheck(pi->delim.at);
        cmpp_atdelim_pop(args->pp);
      }
    }else{
      bool const pushedDelim = NULL!=argDelimC;
      assert( args->d->closer );
      cmpp_dx_consume(dx, NULL, &args->d->closer, 1,
                      cmpp_dx_consume_F_PROCESS_OTHER_D);
      popcheck(pi->policy.at);
      cmpp_atpol_pop(args->pp);
      if( pushedDelim ) cmpp_atdelim_pop(args->pp);
    }
  }else if( op_push==op ){
    if( cmpp_atpol_push(args->pp, polNew) && argDelimC ){
      /* Roll back delimiter push */
      cmpp_atdelim_pop(args->pp);
    }
  }else{
     assert( op_set==op );
     if( cmpp__policy(args->pp,at)!=polNew ){
       cmpp_atpol_set(args->pp, polNew);
     }
  }
end:
  return;
bad_pop:
  cmpp_dx_errf(dx, CMPP_RC_RANGE,
                  "Cannot pop an empty stack.");
#undef popcheck
}
/* …（同文件无关代码省略）… */
static void cmpp_f_expr(cmpp_f_args const * args){
  cmpp * const pp = args->pp;
  cmpp_dx * const dx = args->dx;
  cmpp_d const * const d = args->d;
  unsigned char const * z = 0;
  cmpp_size_t n = 0;
  assert( d );
  if( args->argc<2 ){
    dxserr("An empty expression is not permitted.");
    return;
  }
  cmpp__dx_args_after_name(dx, d, &z, &n);
  cmpp__xval v = cmpp__xval_empty;
  if( 0==cmpp__expr_eval_core(pp, z, n, &v) ){
    if( 'a'==d->name.z[0] ){
      if( !cmpp__xval_truth(&v) ){
        cmpp_errf(pp, CMPP_RC_ASSERT, "Assertion failed: %.*s",
                  (int)n, z);
      }
    }else{
      int64_t i = 0;
      if( 0==cmpp__xval_to_int(pp, &v, &i) ){
        char buf[32];
        int const nb = snprintf(buf, sizeof(buf), "%" PRIi64 "\n", i);
        cmpp_out_raw(pp, buf, (cmpp_size_t)nb);
      }
    }
  }
  cmpp__xval_clear(pp, &v);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_undef_policy(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  cmpp_unpol_e up = cmpp_unpol_invalid;
  int nSeen = 0;
  assert( args->argc>0 && "v4 args" );
  enum ops { op_invalid, op_set, op_push, op_pop };
  enum ops op = op_invalid;
  if( 1==args->argc ){
    cmpp_f_v4_err_usage(args);
    return;
  }
  unsigned i = 1;
  cmpp_arg const * arg;
  cmpp_arg const * argPol = 0;
again:
  arg = args->argv[i];
  ++nSeen;
  if( 1==nSeen ){
    if( cmpp_arg_equals_c(arg, "push") ){
      if( ++i==args->argc ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "Expecting argument to 'push'.");
        return;
      }
      op = op_push;
      goto again;
    }else if( cmpp_arg_equals_c(arg, "pop") ){
      if( ++i < args->argc ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                     "Extra argument after 'pop': %s",
                     cmpp_arg_cstr(args->argv[i]));
        return;
      }
      op = op_pop;
    }
  }
  if( op_pop!=op ){
    if( cmpp_arg_equals_c(arg,"error") ){
      if( op_invalid==op ) op = op_set;
      argPol = arg;
      up = cmpp_unpol_ERROR;
    }else if( cmpp_arg_equals_c(arg,"null") ){
      if( op_invalid==op ) op = op_set;
      argPol = arg;
      up = cmpp_unpol_NULL;
    }else{
      cmpp_errf(args->pp, CMPP_RC_MISUSE,
                "Unhandled argument: %s",
                cmpp_arg_cstr(arg));
      return;
    }
  }
  assert( op_invalid!=op );
  if( op_pop!=op && cmpp_unpol_invalid==up ){
    if( argPol ){
      cmpp_errf(args->pp, CMPP_RC_MISUSE,
                "Unhandled undefined-policy '%s'."
                " Try one of: error, null",
                cmpp_arg_cstr(argPol));
    }else{
      cmpp_f_v4_err_usage(args);
    }
    return;
  }
  switch( op ){
    case op_set:  cmpp_unpol_set(args->pp, up);  break;
    case op_push: cmpp_unpol_push(args->pp, up); break;
    case op_pop:
      if( !cmpp__epol(args->pp,un).n ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                     "No %s%s push is active.",
                     cmpp_dx_delim(dx), args->d->name.z);
      }else{
        cmpp_unpol_pop(args->pp);
      }
      break;
    default:
      cmpp__fatal("Not possible - unhandled switch case");
  }
}
/* …（同文件无关代码省略）… */
static void cmpp_f_delimiter(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  enum ops { op_none, op_set, op_push, op_pop };
  enum ops op = op_none;
  cmpp_arg const * argD = 0;
  bool doHeredoc = false;
  for( unsigned i = 1; i < args->argc; ++i ){
    cmpp_arg const * const arg = args->argv[i];
    if( op_none==op ){
      /* Look for push|pop. */
      if( cmpp_arg_equals_c(arg, "push") ){
        op = op_push;
        continue;
      }else if( cmpp_arg_equals_c(arg, "pop") ){
        if( i+1<args->argc ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "'pop' expects no arguments.");
          return;
        }
        op = op_pop;
        break;
      }
      /* Fall through */
    }
    if( !argD ){
      if( op_none==op ) op = op_set;
      argD = arg;
      continue;
    }else if( !doHeredoc && cmpp_arg_equals_c(arg,"<<") ){
      if( args->isDxCall ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "'%s' is not legal in [call] form.", arg->z);
        return;
      }else if( i+1<args->argc ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "'%s' must be the final argument.", arg->z);
          return;
      }
      op = op_push;
      doHeredoc = true;
      continue;
    }
    cmpp_dx_errf(dx, CMPP_RC_MISUSE, "Unhandled arg: %s", arg->z);
    return;
  }
  if( op_pop==op ){
    cmpp_delimiter_pop(args->pp);
  }else if( !argD ){
    if( args->isDxCall ){
      cmpp__delim const * const del = cmpp__dx_delim(dx);
      if( del ) cmpp_out_raw(args->pp, del->open.z, del->open.n);
    }else{
      cmpp_errf(args->pp, CMPP_RC_MISUSE, "No delimiter specified.");
    }
    return;
  }else{
    char const * const z =
      (0==strcmp("default",(char*)argD->z))
      ? NULL
      : (char const*)argD->z;
    if( op_push==op ){
      /* Includes the doHeredoc case. */
      cmpp_delimiter_push(args->pp, z);
    }else{
      assert( op_set==op );
      assert( !doHeredoc );
      cmpp_delimiter_set(args->pp, z);
    }
  }
  if( !cmpp_err_has(dx->pp) ){
    if( args->isDxCall ){
      cmpp__delim const * const del = cmpp__dx_delim(dx);
      if( del ) cmpp_out_raw(args->pp, del->open.z, del->open.n);
    }else if( doHeredoc ){
      assert( op_push==op );
      cmpp_dx_consume(dx, NULL, &args->d->closer, 1,
                      cmpp_dx_consume_F_PROCESS_OTHER_D);
      cmpp_delimiter_pop(args->pp);
    }
  }
}
/* …（同文件无关代码省略）… */
#define nextArg (++*i<args->argc ? args->argv[*i] : 0)
/* …（同文件无关代码省略）… */
    cmpp_arg const *pN = nextArg;
    if( pN && cmpp_arg_equals_c(pN, "bind") ){
      pN = nextArg;
      if( !pN || (
            cmpp_tt_GroupSquiggly!=pN->ttype
            && cmpp_tt_GroupBrace!=pN->ttype
          ) ){
        return serr("Expecting {...} or [...] after 'bind'.");
      }
      *pBind = pN;
    }else{
/* …（同文件无关代码省略）… */
static int cmpp_kav_each_f_query__bind(
  cmpp * const pp,
  unsigned char const * const zKey, cmpp_size_t nKey,
  unsigned char const * const zVal, cmpp_size_t nVal,
  void * const callbackState
){
  /* Expecting: :bindName -> bindValue */
  if( ':'!=zKey[0] && '$'!=zKey[0] && '@'!=zKey[0] ){
    cmpp_errf(pp, CMPP_RC_MISUSE,
                 "Bind keys must start with ':' or '$' or '@'.");
  }else{
    sqlite3_stmt * const q = callbackState;
    assert( q );
    int const bindNdx =
      sqlite3_bind_parameter_index(q, (char const*)zKey);
    if( bindNdx ){
      cmpp_stmt_bind_textn(pp, q, bindNdx, zVal, nVal);
    }else{
      cmpp_errf(pp, CMPP_RC_RANGE, "Invalid bind name: %.*s",
                   (int)nKey, zKey);
    }
  }
  return ppCode;
}

int cmpp__bind_group(cmpp * const pp, sqlite3_stmt * const q,
                     cmpp_arg const * const aGroup){
  if( ppCode ) return ppCode;
  if( cmpp_tt_GroupSquiggly==aGroup->ttype ){
    return cmpp_kav_each(
      pp, aGroup->z, aGroup->n,
      cmpp_kav_each_f_query__bind, q,
      cmpp_kav_each_F_NOT_EMPTY
      | cmpp_kav_each_F_CALL_VAL
      | cmpp_kav_each_F_PARENS_EXPR
    );
  }
  if( cmpp_tt_GroupBrace!=aGroup->ttype ){
    return cmpp_errf(pp, CMPP_RC_MISUSE,
                        "Expecting {...} or [...] "
                        "for SQL binding list.");
  }
  int bindNdx = 0;
  cmpp_args args = cmpp_args_empty;
  cmpp_args_parse(pp, &args, aGroup->z, aGroup->n, 0);
  if( !args.argc && !ppCode ){
    cmpp_errf(pp, CMPP_RC_RANGE,
                 "Empty SQL bind list is not permitted.");
    /* Keep going so we can clean up a partially-parsed args. */
  }
  for( cmpp_arg const * aVal = args.arg0;
       !ppCode && aVal;
       aVal = aVal->next ){
    ++bindNdx;
    if( 0 ){
      g_warn("bind #%d %s <<%s>>", bindNdx,
             cmpp__tt_cstr2(aVal->ttype, true), aVal->z);
    }
    cmpp__bind_arg(pp, q, bindNdx, aVal);
  }
  cmpp_args_cleanup(&args);
  return ppCode;
}

/* …（同文件无关代码省略）… */
static void cmpp_f_sum(cmpp_f_args const * args){
  int64_t nTotal = 0, nn = 0;
  cmpp_b * const b = cmpp_b_borrow(args->pp);
  if( !b ) return;
  int rc = b ? 0 : CMPP_RC_OOM;
  for( unsigned i = 1; 0==rc && i < args->argc; ++i ){
    cmpp_arg const * arg = args->argv[i];
    rc = cmpp_arg_interpolate(args->pp, arg, cmpp_b_reuse(b),
                       cmpp_arg_interpolate_BRACE_CALL);
    if( 0==rc && cmpp_is_int64(b->z, b->n, &nn) ){
      nTotal += nn;
    }
  }
  if( 0==rc ){
    cmpp_b_append_i64(cmpp_b_reuse(b), nTotal);
    cmpp_out_raw(args->pp, b->z, b->n);
  }
  cmpp_b_return(args->pp, b);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_arg(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  cmpp_flag32_t a2bFlags = cmpp_arg_interpolate_BRACE_CALL;
  bool trimL = false, trimR = false;
  if( args->argc<2 ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE, "Expecting an argument.");
    return;
  }
  //g_warn("argc=%u", args->argc);
  cmpp_b * const b = cmpp_b_borrow(args->pp);
  if( !b ) return;
  for( unsigned i = 1; i < args->argc && !cmpp_err_has(args->pp); ++i ){
    cmpp_arg const * const arg = args->argv[i];
#define FLAG(X)if( cmpp_arg_isflag_c(arg, X) )
    FLAG("-raw") {
      a2bFlags = cmpp_arg_interpolate_FORCE_STRING;
      continue;
    }
    FLAG("-trim-left")  { trimL=true; continue; }
    FLAG("-trim-right") { trimR=true; continue; }
    FLAG("-trim")       { trimL=trimR=true; continue; }
#undef FLAG
    if( 0==cmpp_arg_interpolate(args->pp, arg, cmpp_b_reuse(b), a2bFlags) ){
      unsigned char const * zz = b->z;
      unsigned char const * zzEnd = b->z + b->n;
      if( trimL ) cmpp_skip_snl(&zz, zzEnd);
      if( trimR ) cmpp_skip_snl_trailing(zz, &zzEnd);
      if( 0 ){
        g_warn("arg n=%d bn=%d %s %.*s",
               (int)arg->n, (int)b->n, cmpp_tt_cstr(arg->ttype),
               (int)(zzEnd-zz), zz);
      }
      if( zzEnd-zz>0 ){
        cmpp_out_raw(args->pp, zz, zzEnd-zz);
        //g_warn0("here");
      }
    }
  }
  cmpp_b_return(args->pp, b);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_join(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  cmpp_b * const b = cmpp_b_borrow(args->pp);
  cmpp_b * const bSep = cmpp_b_borrow(args->pp);
  cmpp_flag32_t a2bFlags = cmpp_arg_interpolate_BRACE_CALL;
  bool addNl = !args->isDxCall;
  int n = 0;
  if( !b || !bSep ) goto end;
  if( args->argc<2 ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                    "%s%s expects ?flags? ...args",
                    cmpp_dx_delim(dx), args->d->name.z);
    goto end;
  }
  cmpp_b_append_ch(bSep, ' ');
  cmpp_check_oom(args->pp, bSep->z);
  for( unsigned i = 1; i < args->argc
         && !b->errCode
         && !bSep->errCode
         && !cmpp_err_has(args->pp);
       ++i ){
    cmpp_arg const * const arg = args->argv[i];
#define FLAG(X)if( cmpp_arg_isflag_c(arg, X) )
    FLAG("-s"){
      if( ++i==args->argc ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Missing SEPARATOR argument to -s.");
        break;
      }
      cmpp_arg_interpolate(args->pp, args->argv[i],
                    cmpp_b_reuse(bSep),
                    cmpp_arg_interpolate_BRACE_CALL);
      /* FIXME: unescape bSep */
      continue;
    }
    //FLAG("-nl"){ addNl=true; continue; }
    FLAG("-nonl"){ addNl=false; continue; }
#undef FLAG
    if( n++ && cmpp_out_raw(args->pp, bSep->z, bSep->n) ){
      break;
    }
    if( cmpp_arg_interpolate(args->pp, arg, cmpp_b_reuse(b), a2bFlags) ){
      break;
    }
    cmpp_out_raw(args->pp, b->z, b->n);
  }
  if( !cmpp_err_has(args->pp) ){
    if( !n ){
      cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                      "Expecting at least one argument.");
    }else if( addNl ){
      cmpp_out_raw(args->pp, "\n", 1);
    }
  }
end:
  cmpp_b_return(args->pp, b);
  cmpp_b_return(args->pp, bSep);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_file(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  enum e_op {
    op_none, op_exists, op_join
  };
  cmpp_b * const b0 = cmpp_b_borrow(args->pp);
  if( !b0 ) goto end;
  enum e_op op = op_none;
  cmpp_arg const * arg = 0;
#define nextArg if( ++i==args->argc ) goto missing_arg; arg = args->argv[i]
  unsigned i = 1;
  for(; i < args->argc; ++i ){
    arg = args->argv[i];
    if( op_none==op ){
      if( cmpp_arg_equals_c(arg, "exists") ){
        nextArg;
        op = op_exists;
        if( i+1<args->argc ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "%s%s exists: too many arguments",
                          cmpp_dx_delim(dx), args->d->name.z);
          goto end;
        }
        break;
      }else if( cmpp_arg_equals_c(arg, "join") ){
        nextArg;
        op = op_join;
        break;
      }else{
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Unknown %s%s command: %s",
                  cmpp_dx_delim(dx), args->d->name.z, arg->z);
        goto end;
      }
      cmpp_errf(args->pp, CMPP_RC_MISUSE,
                "%s%s unhandled argument: %s",
                cmpp_dx_delim(dx), args->d->name.z, arg->z);
      goto end;
    }
  }
  cmpp_flag32_t const bFlags = cmpp_arg_interpolate_BRACE_CALL;
  switch( op ){
    case op_none: goto missing_arg;
    case op_join: {
      assert( arg );
      for( int j = 0; arg; ++j ){
        if( cmpp_arg_interpolate(args->pp, arg, cmpp_b_reuse(b0), bFlags)
            || (j && cmpp_out_raw(args->pp, "/", 1))
            || (b0->n && cmpp_out_raw(args->pp, b0->z, b0->n)) ){
          break;
        }
        arg = ++i==args->argc ? 0 : args->argv[i]
          /* The internals actually NULL-pad argv, so we don't _need_
             to do that ternary, but that detail is not part of the
             public API and i'm not yet sure whether it should be. */;
      }
      cmpp_out_raw(args->pp, "\n", 1);
      break;
    }
    case op_exists: {
      assert( arg );
      if( 0==cmpp_arg_interpolate(args->pp, arg, cmpp_b_reuse(b0), bFlags) ){
        bool const b = cmpp__file_is_readable((char const *)b0->z);
        cmpp_out_raw(args->pp, b ? "1\n" : "0\n", 2);
      }
      break;
    }
  }
#undef nextArg
end:
  cmpp_b_return(args->pp, b0);
  return;
missing_arg:
  if( arg ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE, "%s%s %s: missing argument",
              cmpp_dx_delim(dx), args->d->name.z, arg->z );
  }else{
    cmpp_errf(args->pp, CMPP_RC_MISUSE, "%s%s: missing subcommand",
              cmpp_dx_delim(dx), args->d->name.z);
  }
  goto end;
}
/* …（同文件无关代码省略）… */
static void cmpp_f_cmp(cmpp_f_args const * args){
  cmpp_b * const bL = cmpp_b_borrow(args->pp);
  cmpp_b * const bR = cmpp_b_borrow(args->pp);
  cmpp_flag32_t a2bFlags = cmpp_arg_interpolate_BRACE_CALL;
  if( !bL || !bR ) goto end;
  if( 3!=args->argc ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE, "Usage: LHS RHS");
    goto end;
  }
  if( cmpp_arg_interpolate(args->pp, args->argv[1], bL, a2bFlags)
      || cmpp_arg_interpolate(args->pp, args->argv[2], bR, a2bFlags) ){
    goto end;
  }
  int const cmp = strcmp(bL->z ? (char const*)bL->z : "",
                         bR->z ? (char const*)bR->z : "");
  char const * const z = cmp<0 ? "-1" : (cmp ? "1" : "0");
  cmpp_out_raw(args->pp, z, strlen(z));

end:
  cmpp_b_return(args->pp, bL);
  cmpp_b_return(args->pp, bR);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_base64(cmpp_f_args const * args){
  bool comma = false;
  bool quote = false;
  cmpp_arg const * argFile = 0;
  bool preprocessFileArg = false;
  cmpp_b * const ob = cmpp_b_borrow(args->pp);
  if( !ob ){
    goto end;
  }
  for( unsigned i = 1; i < args->argc; ++i ){
    cmpp_arg const * arg = args->argv[i];
#define M(STR) cmpp_arg_equals_c(arg, STR)
    /* -d/-decode is TODO, pending only an actual use case. */
    if( M("-comma") ) comma = true;
    else if( M("-quote") ) quote = true;
    else if( M("-f") || M("-file")
               || (preprocessFileArg=M("-F")) || (preprocessFileArg=M("-FILE"))
    ){
      if( ++i==args->argc ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Expecting an argument after %s.", arg->z);
        goto end;
      }
      argFile = arg = args->argv[i];
      continue;
    }else{
      cmpp_errf(args->pp, CMPP_RC_MISUSE,
                "Unhandled argument: %s", arg->z);
      goto end;
    }
#undef M
  }

  if( argFile ){
    if(cmpp_arg_consume_as_file(args->dx, argFile, NULL, ob,
                                preprocessFileArg) ){
      goto end;
    }
  }else{
    if( args->isDxCall ){
      cmpp_errf(args->pp, CMPP_RC_MISUSE,
                      "#%s...#/%s is not legal for a [call]. "
                      "Pass the -f FILENAME flag instead.",
                      args->d->name.z, args->d->name.z);
      goto end;
    }else{
      assert( !ob->n );
      if( cmpp_dx_consume_b(args->dx, ob, &args->d->closer, 1,
                            cmpp_dx_consume_F_PROCESS_OTHER_D) ){
        goto end;
      }
    }
  }

  if( ob->n ){
    cmpp_b * const b64 = cmpp_b_borrow(args->pp);
    if( !b64 ) goto end;
    int cmpp__b_base64_encode(cmpp *, cmpp_b const *, cmpp_b *)/* in b.c */;
    cmpp__b_base64_encode(args->pp, ob, b64);
    cmpp_b_swap(ob, b64);
    cmpp_b_return(args->pp, b64);
    if( cmpp_err_has(args->pp) ) goto end;
  }
  unsigned char const * zEnd = ob->z + ob->n;
#define out(P,N) if( cmpp_out_raw(args->pp, P, N) ) goto end
  if( !quote ){
    cmpp_out_raw(args->pp, ob->z, ob->n);
    goto end;
  }
  for( unsigned char const * z = ob->z; z < zEnd; ++z ){
    if( quote ){
      out("\"", 1);
    }
    bool closed = false;
    for( ;z<zEnd;++z ){
      if( '\n'==*z ){
        if( quote ){
          if( comma && z+1<zEnd ){
            out("\\n\",\n",5);
          }else{
            out("\\n\"\n",4);
          }
        }else{
          out(z, 1);
        }
        closed = true;
        break;
      }else{
        out(z, 1);
      }
    }
    if( quote && !closed ){
      out("\\n\"\n",4);
    }
  }
#undef out
end:
  cmpp_b_return(args->pp, ob);
  return;
}


/**
   If zName matches one of the delayed-load directives, that directive
   is registered and 0 is returned. CMPP_RC_NO_DIRECTIVE is returned if
/* …（同文件无关代码省略）… */
#define M_OC(NAME) (M(NAME) || M("/" NAME))
#define M_IF(NAME) if( M(NAME) )
#define CF(X) cmpp_f_F_ ## X
#define F_A_RAW CF(ARGS_RAW)
#define F_A_V4 CF(ARGS_V4)
#define F_UNSAFE cmpp_f_F_NOT_IN_SAFEMODE
#define F_NC cmpp_f_F_NO_CALL
#define F_CALL cmpp_f_F_CALL_ONLY
#define DREG0(SYMNAME, NAME, OPENER, OFLAGS, CLOSER, CFLAGS) \
  cmpp_d_reg SYMNAME = {  \
/* …（同文件无关代码省略）… */
#define DREG(NAME, OPENER, OFLAGS, CLOSER, CFLAGS )         \
  DREG0(const rReg, NAME, OPENER, OFLAGS, CLOSER, CFLAGS ); \
  rc = cmpp_d_register(pp, &rReg, NULL);                    \
  goto end
/* …（同文件无关代码省略）… */
#define M_IF_CORE(N,OPENER,OFLAGS,CLOSER,CFLAGS)  \
  if( M_OC(N) ){                                  \
    DREG(N, OPENER, OFLAGS, CLOSER, CFLAGS);      \
  } (void)0

  M_IF_CORE("@",                cmpp_f_at,           F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("arg",              cmpp_f_arg,          F_A_V4, 0, 0);
  M_IF_CORE("assert",           cmpp_f_expr,         F_A_V4, 0, 0);
  M_IF_CORE("base64",           cmpp_f_base64,       F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("cmp",              cmpp_f_cmp,          F_A_V4, 0, 0);
  M_IF_CORE("define",           cmpp_f_define,       F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("delimiter",        cmpp_f_delimiter,    F_A_V4,
/* …（同文件无关代码省略）… */
  M_IF_CORE("expand",           cmpp_f_expand,       F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("expr",             cmpp_f_expr,         F_A_V4, 0, 0);
  M_IF_CORE("join",             cmpp_f_join,         F_A_V4, 0, 0);
  M_IF_CORE("once",             cmpp_f_once,
                                F_A_V4 | F_NC | CF(BLOCK_OPEN),
                                cmpp_f_dangling_closer, CF(BLOCK_CLOSE));
  M_IF_CORE("pragma",           cmpp_f_pragma,       F_A_V4, 0, 0);
  M_IF_CORE("savepoint",        cmpp_f_savepoint,    F_A_V4, 0, 0);
  M_IF_CORE("stderr",           cmpp_f_stderr,       F_A_RAW, 0, 0);
  M_IF_CORE("sum",              cmpp_f_sum,          F_A_V4, 0, 0);
  M_IF_CORE("undef",            cmpp_f_undef,        F_A_V4, 0, 0);
  M_IF_CORE("undefined-policy", cmpp_f_undef_policy, F_A_V4, 0, 0);
  M_IF_CORE("//",               cmpp_f_noop,         F_A_V4 | F_A_RAW,
                                0, 0);
  M_IF_CORE("file",             cmpp_f_file,
                                F_A_V4 | F_UNSAFE, 0, 0);

#undef M_IF_CORE

/* …（同文件无关代码省略）… */
#define M_IF_FLAGGED(NAME,FLAG,OPENER,OFLAGS,CLOSER,CFLAGS) \
  M_IF(NAME) {                                \
    if( 0==(FLAG & pp->pimpl->flags.ctorFlags) ) {    \
      DREG(NAME,OPENER,OFLAGS,CLOSER,CFLAGS); \
    }                                         \
    goto end;                                 \
  }

#ifndef CMPP_OMIT_D_INCLUDE
  M_IF_FLAGGED("include", cmpp_ctor_F_NO_INCLUDE,
               cmpp_f_include, F_A_V4 | F_UNSAFE,
               0, 0);
#endif

/* …（同文件无关代码省略）… */
#endif /*CMPP_OMIT_D_DB*/


#undef M_IF_FLAGGED

#ifndef NDEBUG
/* …（同文件无关代码省略）… */
#undef F_A_V4
#undef F_UNSAFE
#undef F_NC
#undef F_CALL
#undef CF
#undef M
/* …（同文件无关代码省略）… */
bool cmpp__file_is_readable(char const *zFile){
  return 0==access(zFile, R_OK);
}
/* …（同文件无关代码省略）… */
  return ppCode;
}

CMPP__EXPORT(int, cmpp_arg_interpolate)(cmpp * const pp, cmpp_arg const *arg,
                                        cmpp_b * ob, cmpp_flag32_t flags){
  /**
     Reminder to self: this function specifically does not do any
     expression evaluation of its arguments. Please avoid the
     temptation to make it do so. Unless it proves necessary.  Or
     useful. Even then, though, consider the implications deeply
     before doing so.

     2026-09-21: The above still currently applies but we're going to
     want expressions if/when the v1 expression cruft is replaced with
     v2.
  */
  if( !ob ){
    if( arg->interp ){
      ob = cmpp_b_reuse(arg->interp);
    }else{
      return cmpp_errf(pp, CMPP_RC_MISUSE,
                       "%s() only accepts a NULL cmpp_b if arg->interp "
                       "is not NULL.", __func__);
    }
  }
  switch( ppCode
          ? 0
/* …（同文件无关代码省略）… */
    case 0:
      break;
    case cmpp_tt_Word:
      if( 0==(flags & cmpp_arg_interpolate_NO_DEFINES) ){
        cmpp__get_b(pp, arg->z, arg->n, ob, true);
        break;
      }
      goto theDefault;
    case cmpp_tt_GroupBrace:
      if( !(cmpp_arg_interpolate_NO_BRACE_CALL & flags)
          && (cmpp_arg_interpolate_BRACE_CALL & flags) ){
/* …（同文件无关代码省略）… */
int cmpp__bind_arg(cmpp * const pp, sqlite3_stmt * const q,
                   int bindNdx, cmpp_arg const * const arg){

  if( 0 ){
    g_warn("bind #%d %s <<%.*s>>", bindNdx,
           cmpp__tt_cstr2(arg->ttype, true),
           (int)arg->n, arg->z);
  }
  switch( arg->ttype ){
    default:
    case cmpp_tt_IntDec:
    case cmpp_tt_StringBT:
    case cmpp_tt_StringDQ:
    case cmpp_tt_StringSQ:
      cmpp_stmt_bind_textn(pp, q, bindNdx, arg->z, (int)arg->n);
      break;

    case cmpp_tt_Word:{
      cmpp_b os = cmpp_b_empty;
      if( 0==cmpp_arg_interpolate(pp, arg, &os, 0) ){
        if( 0 ){
          g_warn("bind #%d <<%s>> => <<%.*s>>",
                 bindNdx, arg->z, (int)os.n, os.z);
        }
        cmpp_stmt_bind_textn(pp, q, bindNdx, os.z, (int)os.n);
      }
      cmpp_b_clear(&os);
      break;
    }

    case cmpp_tt_GroupParen:{
      int64_t i = 0;
      if( 0==cmpp__expr_eval_paren(pp, arg, &i) ){
        /* See comment above about cmpp_tt_Int. */
        cmpp_stmt_bind_int_text(pp, q, bindNdx, i);
      }
      break;
    }

    case cmpp_tt_GroupBrace:{
      cmpp_b b = cmpp_b_empty;
      cmpp_call_str(pp, arg->z, arg->n, &b, 0);
      cmpp_stmt_bind_textn(pp, q, bindNdx, b.z, b.n);
      cmpp_b_clear(&b);
      break;
    }

  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
  return ppCode;
}

/* Helper for cmpp_kav_each() and friends. */
static
int cmpp__each_paren_expr(cmpp *pp, cmpp_arg const * arg,
                          unsigned char * pOut, size_t nOut){
  int64_t d = 0;
  if( 0==cmpp__expr_eval_paren(pp, arg, &d) ){
    snprintf((char *)pOut, nOut, "%" PRIi64, d);
  }
  return ppCode;
}

CMPP__EXPORT(int, cmpp_kav_each)(cmpp *pp,
                                 unsigned char const *zBegin,
                                 cmpp_strlen_t nIn,
/* …（同文件无关代码省略）… */
  cmpp_b * const bVal = bKey ? cmpp_b_borrow(pp) : 0;
  if( !bVal ) goto end;
  bool const reqArrow = 0==(cmpp_kav_each_F_NO_ARROW & flags);
  unsigned char exprBuf[32] = {0};
  cmpp_size_t const nz = cmpp__strlenu(zBegin,nIn);
  cmpp_flag32_t a2bK = 0, a2bV = 0 /*cmpp_arg_interpolate() flags*/;
  assert( zBegin );
/* …（同文件无关代码省略）… */
    a2bV |= cmpp_arg_interpolate_BRACE_CALL;
    flags |= cmpp_kav_each_F_EXPAND_VAL;
  }
  cmpp_arg const * aNext = 0;
  for( cmpp_arg const * aKey = args.arg0;
       !ppCode && aKey;
/* …（同文件无关代码省略）… */
      zKey = aKey->z;
      nKey = aKey->n;
    }
    if( cmpp_tt_GroupParen==aVal->ttype
        && (cmpp_kav_each_F_PARENS_EXPR & flags) ){
      if( cmpp__each_paren_expr(pp, aVal, &exprBuf[0],
                                sizeof(exprBuf)-1) ){
        break;
      }
      zVal = &exprBuf[0];
      nVal = cmpp__strlenu(zVal, -1);
    }else if( cmpp_kav_each_F_EXPAND_VAL & flags ){
      if( cmpp_arg_interpolate(pp, aVal, cmpp_b_reuse(bVal),
                             a2bV) ){
        break;
/* …（同文件无关代码省略）… */
     those same buffers. */
  cmpp_b ob = cmpp_b_empty;
  cmpp_args args = cmpp_args_empty;
  unsigned char exprBuf[32] = {0};
  cmpp_size_t const nz = cmpp__strlenu(zBegin,nIn);
  assert( zBegin );

/* …（同文件无关代码省略）… */
    /* Expand the key/value parts if needed... */
    unsigned char const *zVal;
    cmpp_size_t nVal;
    if( cmpp_tt_GroupParen==arg->ttype
        && (cmpp_kav_each_F_PARENS_EXPR & flags) ){
      if( cmpp__each_paren_expr(pp, arg, &exprBuf[0],
                                sizeof(exprBuf)-1) ){
        break;
      }
      zVal = &exprBuf[0];
      nVal = cmpp__strlenu(zVal, -1);
    }else if( cmpp_kav_each_F_EXPAND_VAL & flags ){
      if( cmpp_arg_interpolate(pp, arg, cmpp_b_reuse(&ob), 0) ){
        break;
      }
      zVal = ob.z;
/* …（同文件无关代码省略）… */
int cmpp_arg_consume_as_file(
  cmpp_dx * dx, cmpp_arg const *arg,
  cmpp_b * bNameOut, cmpp_b * content, bool preprocessIt
){
  if( !arg ){
    return cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                           "Expecting filename argument.");
  }
  cmpp_b * const bFile = cmpp_b_borrow(dx->pp);
  if( !bFile ) return dxppCode;
  else if( cmpp_arg_interpolate(dx->pp, arg, bFile,
                         cmpp_arg_interpolate_BRACE_CALL) ){
    goto end;
  }
  if( !bFile->n || !bFile->z[0] ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE, "Empty filename is not permitted.");
    goto end;
  }
  cmpp_FILE * const fp =
    cmpp_fopen((char const*)bFile->z, "r")
    /* Resolving as -Idir does would arguably be a bug. */;
  if( fp ){
    cmpp_outputer out = cmpp_outputer_b;
    int rc;
    out.state = cmpp_b_reuse(content);
    if( preprocessIt ){
      cmpp_outputer oOld = {0};
      cmpp__outputer_swap(dx->pp, &out, &oOld);
      rc = cmpp_process_stream(dx->pp, (char const *)bFile->z,
                               cmpp_input_f_FILE, fp);
      cmpp__outputer_swap(dx->pp, &oOld, NULL);
    }else{
      rc = cmpp_stream(cmpp_input_f_FILE, fp, out.out, out.state);
    }
    if( rc && !cmpp_err_has(dx->pp) ){
      /* Propagate error. */
      cmpp_dx_errf(dx, rc, "Unknown error streaming file [%s].",
                      bFile->z);
    }
    cmpp_fclose(fp);
  }else{
    cmpp_dx_errf(dx, cmpp_errno_rc(errno, CMPP_RC_IO),
                    "Unknown error opening file %s.", bFile->z);
  }
 end:
  if( bNameOut && bFile && !dxppCode ){
    cmpp_b_swap(bFile, bNameOut);
  }
  cmpp_b_return(dx->pp, bFile);
  return dxppCode;
}
/* …（同文件无关代码省略）… */
typedef struct CmppXP CmppXP;
/* …（同文件无关代码省略）… */
static int cmpp__xp_rc(CmppXP const * const p){
  return p->ex->xErrCode
    ? p->ex->xErrCode(p->ex)
    : p->pp->pimpl->err.code;
}
#define xpCode cmpp__xp_rc(p)
/* …（同文件无关代码省略）… */
static int cmpp__xp_err(CmppXP * const p, unsigned char const * zAt,
                        int rc, char const *zFmt, ...){
  cmpp * const pp = p->pp;
  int const rcOld = xpCode;
  if( rcOld ) return rcOld;
  va_list va;
  va_start(va, zFmt);
  if( p->ex->xErr ){
    char * const zMsg = sqlite3_vmprintf(zFmt, va);
    if( zMsg ){
      rc = p->ex->xErr(p->ex, p->tz, zAt, rc, zMsg);
      sqlite3_free(zMsg);
    }else{
      rc = cmpp_err(pp, CMPP_RC_OOM, 0, 0);
    }
  }else{
    cmpp_dx * const dx = pp->pimpl->dx;
    unsigned char const * zOld = 0;
    if( dx && zAt ){
      zOld = dx->pimpl->zErrAt;
      dx->pimpl->zErrAt = zAt;
    }
    rc = cmpp_errfv(pp, rc, zFmt, va);
    if( dx && zAt ){
      dx->pimpl->zErrAt = zOld;
    }
  }
  va_end(va);
  return rc;
}
/* …（同文件无关代码省略）… */
static int cmpp__xp_classify(cmpp_token const * const t){
  unsigned char const * const z = t->z;
  switch( t->ttype ){
    case cmpp_tt_Word: break;
    case cmpp_tt_IntDec:
      /* The tokenizer reports a lone + or - as IntDec. */
      if( 1==t->n && ('-'==*z || '+'==*z) ) break;
      return XOp_none;
    case cmpp_tt_OpNot:
      /* The tokenizer tags runs of ! as OpNot. */
      break;
    default:
      return XOp_none;
  }
  switch( t->n ){
    case 1:
      switch( *z ){
        case '=': return XOp_eq;
        case '<': return XOp_lt;
        case '>': return XOp_gt;
        case '!': return XOp_bang;
        case '+': return XOp_plus;
        case '-': return XOp_minus;
        case '*': return XOp_mul;
        case '/': return XOp_div;
        case '%': return XOp_mod;
        case '&': case '|': case '~': case '^':
          return XOp_unsupported;
      }
      return XOp_none;
    case 2:
#define M(A,B) (A==z[0] && B==z[1])
      if( M('=','=') ) return XOp_eq;
      if( M('!','=') ) return XOp_ne;
      if( M('<','=') ) return XOp_le;
      if( M('>','=') ) return XOp_ge;
      if( M('&','&') ) return XOp_and;
      if( M('|','|') ) return XOp_or;
      if( M('o','r') ) return XOp_or;
      if( M('i','n') ) return XOp_in;
      if( M('<','>') || M('<','<') || M('>','>') ) return XOp_unsupported;
#undef M
      break;
    case 3:
      if( 0==memcmp(z, "and", 3) ) return XOp_and;
      if( 0==memcmp(z, "not", 3) ) return XOp_not;
      if( 0==memcmp(z, "<<<", 3) ) return XOp_unsupported;
      break;
    case 4:
      if( 0==memcmp(z, "glob", 4) ) return XOp_glob;
      break;
    case 7:
      if( 0==memcmp(z, "defined", 7) ) return XOp_defined;
      break;
  }
  if( '!'==*z ){
    /* A run of !!! is a run of negations. */
    unsigned char const * zz = z;
    unsigned char const * const ze = z + t->n;
    while( zz<ze && '!'==*zz ) ++zz;
    if( zz==ze ) return XOp_bang;
  }
  return XOp_none;
}
/* …（同文件无关代码省略）… */
static int cmpp__xp_lbp(int op){
  switch( op ){
    case XOp_or: return BP_OR;
    case XOp_and: return BP_AND;
    case XOp_eq: case XOp_ne:
    case XOp_glob: case XOp_notGlob:
    case XOp_in: case XOp_notIn:
      return BP_EQ;
    case XOp_lt: case XOp_le: case XOp_gt: case XOp_ge:
      return BP_REL;
    case XOp_plus: case XOp_minus: return BP_ADD;
    case XOp_mul: case XOp_div: case XOp_mod: return BP_MUL;
    default: return 0;
  }
}
/* …（同文件无关代码省略）… */
static int cmpp__xp_next(CmppXP * const p){
  if( cmpp_tizer_next(p->tz, cmpp_tizer_next_SKIP_NL
                      | cmpp_tizer_next_SKIP_C_COMMENTS) ){
    /* Move the tokenizer's error, which lacks location info, into
       the error state, with location info. */
    int rc = p->ex->xTzErr
      ? p->ex->xTzErr(p->ex, p->tz)
      : cmpp__args_tzerr_uplift(p->pp, p->tz);
    if( !rc ){
      rc = cmpp__xp_err(p, p->tz->token.z, CMPP_RC_SYNTAX,
                        "Tokenization error in expression.");
    }
    return rc;
  }
  p->tok = p->tz->token;
  p->op = cmpp__xp_classify(&p->tok);
  return 0;
}
/* …（同文件无关代码省略）… */
static int cmpp__xp_parse_int(unsigned char const *z, cmpp_size_t n,
                              int64_t *pOut){
  unsigned char const * const ze = z + n;
  bool neg = false;
  bool overflow = false;
  uint64_t u = 0;
  if( z<ze && ('-'==*z || '+'==*z) ){
    neg = '-'==*z;
    ++z;
  }
  if( z==ze ) return 0;
  for( ; z<ze; ++z ){
    if( *z<'0' || *z>'9' ) return 0;
    unsigned const d = (unsigned)(*z - '0');
    if( !overflow ){
      if( u > (UINT64_MAX - d) / 10 ) overflow = true;
      else u = u*10 + d;
    }
  }
  uint64_t const lim = (uint64_t)INT64_MAX + (neg ? 1 : 0);
  if( overflow || u>lim ) return -1;
  *pOut = neg ? (int64_t)(0 - u) : (int64_t)u;
  return 1;
}
/* …（同文件无关代码省略）… */
void cmpp__xval_clear(cmpp * const pp, cmpp__xval * const v){
  if( v->b ){
    cmpp_b_return(pp, v->b);
  }
  *v = cmpp__xval_empty;
}
/* …（同文件无关代码省略）… */
static void cmpp__xval_set_int(cmpp * const pp, cmpp__xval * const v,
                               int64_t i){
  cmpp__xval_clear(pp, v);
  v->type = cmpp__xval_INT;
  v->i = i;
}
/* …（同文件无关代码省略）… */
int cmpp__xval_str(cmpp * const pp, cmpp__xval * const v){
  if( ppCode ) return ppCode;
  v->type = cmpp__xval_STR;
  if( !v->b ){
    v->b = cmpp_b_borrow(pp);
    if( !v->b ) return ppCode;
  }
  cmpp_b_reuse(v->b);
  return 0;
}
/* …（同文件无关代码省略）… */
static void cmpp__xval_bytes(cmpp__xval const * const v, char * buf,
                             unsigned char const **pz, cmpp_size_t *pn){
  if( cmpp__xval_INT==v->type ){
    int const n = snprintf(buf, 24, "%" PRIi64, v->i);
    *pz = (unsigned char const *)buf;
    *pn = (cmpp_size_t)n;
  }else if( v->b && v->b->z ){
    *pz = v->b->z;
    *pn = v->b->n;
  }else{
    *pz = (unsigned char const *)"";
    *pn = 0;
  }
}
/* …（同文件无关代码省略）… */
int cmpp__xval_to_int(cmpp * const pp, cmpp__xval const * const v,
                      int64_t * const pOut){
  if( ppCode ) return ppCode;
  if( cmpp__xval_INT==v->type ){
    *pOut = v->i;
    return 0;
  }
  *pOut = 0;
  if( !v->b || !v->b->n ) return 0;
  unsigned char const * z = v->b->z;
  unsigned char const * const zEnd = z + v->b->n;
  cmpp_skip_snl(&z, zEnd);
  unsigned char const * zDigits = z;
  if( zDigits<zEnd && ('-'==*zDigits || '+'==*zDigits) ) ++zDigits;
  unsigned char const * zTail = zDigits;
  while( zTail<zEnd && *zTail>='0' && *zTail<='9' ) ++zTail;
  if( zTail==zDigits ) return 0 /* no leading integer */;
  if( -1==cmpp__xp_parse_int(z, (cmpp_size_t)(zTail - z), pOut) ){
    return cmpp_errf(pp, CMPP_RC_RANGE,
                     "Integer is out of range: %.*s",
                     (int)(zTail - z), z);
  }
  return 0;
}
/* …（同文件无关代码省略）… */
bool cmpp__xval_truth(cmpp__xval const * const v){
  if( cmpp__xval_INT==v->type ) return 0!=v->i;
  if( !v->b ) return false;
  return cmpp_simple_truth(v->b->z, (cmpp_strlen_t)v->b->n);
}
/* …（同文件无关代码省略）… */
int cmpp__expr_operand_core(cmpp__expr const * ex,
                            cmpp_tizer * tz,
                            cmpp_token const * tok,
                            cmpp_flag32_t flags,
                            cmpp__xval * out){
  cmpp * const pp = ex->pp;
  (void)flags;
  if( cmpp__xval_str(pp, out) ) return ppCode;
  switch( tok->ttype ){
    case cmpp_tt_Word:
      cmpp__get_b(pp, tok->z, tok->n, out->b, true);
      break;
    case cmpp_tt_StringSQ:
    case cmpp_tt_StringDQ:
    case cmpp_tt_StringBT:
      if( cmpp_token_content(tz, tok, out->b, 0) ){
        cmpp_err(pp, CMPP_RC_OOM, 0, 0);
      }
      break;
    case cmpp_tt_GroupBrace:
      cmpp_call_str(pp, tok->zInner, tok->nInner, out->b, 0);
      break;
    case cmpp_tt_GroupSquiggly:
      /* Reserved for lists (e.g. as the RHS of "in"). */
      return cmpp_errf(pp, CMPP_RC_SYNTAX,
                       "{...} is not (yet) permitted in expressions.");
    default:
      return cmpp_errf(pp, CMPP_RC_SYNTAX,
                       "Unhandled operand type %s: %.*s",
                       cmpp_tt_cstr(tok->ttype),
                       (int)tok->n, tok->z);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
static int cmpp__xp_prefix(CmppXP * const p, cmpp__xval * const rv){
  cmpp * const pp = p->pp;
  cmpp_token const t = p->tok;
  int const op = p->op;
  switch( op ){
    case XOp_none: break;
    case XOp_not:
    case XOp_bang:{
      /* Count a run of ! for its parity. */
      unsigned nNeg = 1;
      if( XOp_bang==op ) nNeg = t.n;
      if( cmpp__xp_next(p) || cmpp__xp_expr(p, XOp_not==op
                                            ? BP_NOT : BP_PREFIX, rv) ){
        return xpCode;
      }
      if( !p->skip ){
        bool b = cmpp__xval_truth(rv);
        if( nNeg & 1 ) b = !b;
        cmpp__xval_set_int(pp, rv, b);
      }
      return 0;
    }
    case XOp_minus:{
      if( cmpp__xp_next(p) || cmpp__xp_expr(p, BP_PREFIX, rv) ){
        return xpCode;
      }
      if( !p->skip ){
        int64_t i = 0;
        if( !cmpp__xval_intish(rv, &i) ){
          return cmpp__xp_err(p, t.z, CMPP_RC_TYPE,
                              "Non-integer operand for unary '-'.");
        }
        cmpp__xval_set_int(pp, rv, (int64_t)(0 - (uint64_t)i));
      }
      return 0;
    }
    case XOp_defined:{
      if( cmpp__xp_next(p) ) return xpCode;
      if( cmpp_tt_Word!=p->tok.ttype || p->op ){
        return cmpp__xp_err(p, p->tok.z ? p->tok.z : t.z,
                            CMPP_RC_SYNTAX,
                            "'defined' requires a name operand.");
      }
      if( !p->skip ){
        cmpp_token const * const w = &p->tok;
        int got;
        if( w->n>1 && '#'==w->z[0] ){
          /* defined #foo => is directive foo available? */
          cmpp_b * const b = cmpp_b_borrow(pp);
          if( !b ) return xpCode;
          if( 0==cmpp_b_append4(pp, b, w->z+1, w->n-1) ){
            got = !!cmpp__d_search3(pp, (char const*)b->z,
                                    cmpp__d_search3_F_NO_DLL);
          }else{
            got = 0;
          }
          cmpp_b_return(pp, b);
        }else{
          got = cmpp__has(pp, (char const *)w->z, w->n);
        }
        if( xpCode ) return xpCode;
        cmpp__xval_set_int(pp, rv, got);
      }
      return cmpp__xp_next(p);
    }
    case XOp_unsupported:
      return cmpp__xp_err(p, t.z, CMPP_RC_SYNTAX,
                          "Unsupported operator: %.*s",
                          (int)t.n, t.z);
    default:
      return cmpp__xp_err(p, t.z, CMPP_RC_SYNTAX,
                          "Expecting an operand but got operator: %.*s",
                          (int)t.n, t.z);
  }
  /* A plain operand. */
  switch( t.ttype ){
    case cmpp_tt_EOF:
      return cmpp__xp_err(p, t.z, CMPP_RC_SYNTAX,
                          "Expecting an operand but reached "
                          "the end of the expression.");
    case cmpp_tt_GroupParen:
      if( cmpp__xp_group(p, &t, rv) ) return xpCode;
      break;
    case cmpp_tt_IntDec:{
      int64_t i = 0;
      if( 1!=cmpp__xp_parse_int(t.z, t.n, &i) ){
        return cmpp__xp_err(p, t.z, CMPP_RC_RANGE,
                            "Integer is out of range: %.*s",
                            (int)t.n, t.z);
      }
      if( !p->skip ) cmpp__xval_set_int(pp, rv, i);
      break;
    }
    case cmpp_tt_GroupSquiggly:
      /* The core resolver rejects these: they're reserved for lists
         (e.g. as the RHS of "in"). Itch treats them as strings. */
    case cmpp_tt_Word:
    case cmpp_tt_StringSQ:
    case cmpp_tt_StringDQ:
    case cmpp_tt_StringBT:
    case cmpp_tt_GroupBrace:
      if( !p->skip ){
        cmpp_dx * const dx = pp->pimpl->dx;
        unsigned char const * zOld = 0;
        if( dx ){
          /* Errors from the resolver get reported at this token. */
          zOld = dx->pimpl->zErrAt;
          dx->pimpl->zErrAt = t.z;
        }
        int const rc = p->ex->xOperand(p->ex, p->tz, &t, 0, rv);
        if( dx ) dx->pimpl->zErrAt = zOld;
        if( rc ) return rc;
        if( xpCode ) return xpCode;
      }
      break;
    default:
      return cmpp__xp_err(p, t.z, CMPP_RC_SYNTAX,
                          "Unexpected %s token in expression: %.*s",
                          cmpp_tt_cstr(t.ttype), (int)t.n, t.z);
  }
  return cmpp__xp_next(p);
}
/* …（同文件无关代码省略）… */
static void cmpp__xp_compare(CmppXP * const p, int op,
                             cmpp__xval * const lhs,
                             cmpp__xval const * const rhs){
  int64_t a = 0, b = 0;
  int c;
  if( cmpp__xval_intish(lhs, &a) && cmpp__xval_intish(rhs, &b) ){
    c = a<b ? -1 : (a>b);
  }else{
    char bufL[24], bufR[24];
    unsigned char const *zL, *zR;
    cmpp_size_t nL, nR;
    cmpp__xval_bytes(lhs, bufL, &zL, &nL);
    cmpp__xval_bytes(rhs, bufR, &zR, &nR);
    c = memcmp(zL, zR, nL<nR ? nL : nR);
    if( 0==c ) c = nL<nR ? -1 : (nL>nR);
  }
  bool r;
  switch( op ){
    case XOp_eq: r = 0==c; break;
    case XOp_ne: r = 0!=c; break;
    case XOp_lt: r = c<0; break;
    case XOp_le: r = c<=0; break;
    case XOp_gt: r = c>0; break;
    case XOp_ge: r = c>=0; break;
    default: assert(!"not a comparison op"); r = false;
  }
  cmpp__xval_set_int(p->pp, lhs, r);
}
/* …（同文件无关代码省略）… */
static int cmpp__xp_arith(CmppXP * const p, int op,
                          cmpp_token const * const tOp,
                          cmpp__xval * const lhs,
                          cmpp__xval const * const rhs){
  int64_t a = 0, b = 0;
  if( !cmpp__xval_intish(lhs, &a) || !cmpp__xval_intish(rhs, &b) ){
    return cmpp__xp_err(p, tOp->z, CMPP_RC_TYPE,
                        "Non-integer operand for '%.*s'.",
                        (int)tOp->n, tOp->z);
  }
  uint64_t const ua = (uint64_t)a, ub = (uint64_t)b;
  int64_t r = 0;
  switch( op ){
    case XOp_plus:  r = (int64_t)(ua + ub); break;
    case XOp_minus: r = (int64_t)(ua - ub); break;
    case XOp_mul:   r = (int64_t)(ua * ub); break;
    case XOp_div:
    case XOp_mod:
      if( 0==b ){
        return cmpp__xp_err(p, tOp->z, CMPP_RC_RANGE,
                            "Division by zero.");
      }
      if( -1==b ){
        /* Avoid the INT64_MIN / -1 trap. */
        r = XOp_div==op ? (int64_t)(0 - ua) : 0;
      }else{
        r = XOp_div==op ? a / b : a % b;
      }
      break;
    default: assert(!"not an arithmetic op");
  }
  cmpp__xval_set_int(p->pp, lhs, r);
  return 0;
}
/* …（同文件无关代码省略）… */
static int cmpp__xp_expr(CmppXP * const p, int minBp, cmpp__xval * rv){
  cmpp * const pp = p->pp;
  if( cmpp__xp_prefix(p, rv) ) return xpCode;
  cmpp__xval rhs = cmpp__xval_empty;
  while( 0==xpCode ){
    cmpp_token const tOp = p->tok;
    int op = p->op;
    if( cmpp_tt_EOF==tOp.ttype ) break;
    if( XOp_none==op ){
      cmpp__xp_err(p, tOp.z, CMPP_RC_SYNTAX,
                   "Expecting an operator but got: %.*s",
                   (int)tOp.n, tOp.z);
      break;
    }else if( XOp_unsupported==op ){
      cmpp__xp_err(p, tOp.z, CMPP_RC_SYNTAX,
                   "Unsupported operator: %.*s",
                   (int)tOp.n, tOp.z);
      break;
    }
    cmpp_tizer tzSave = cmpp_tizer_empty;
    if( XOp_not==op ){
      /* Infix "not" is only legal as "not glob" and "not in". */
      tzSave = *p->tz;
      if( cmpp__xp_next(p) ) break;
      if( XOp_glob==p->op ) op = XOp_notGlob;
      else if( XOp_in==p->op ) op = XOp_notIn;
      else{
        cmpp__xp_err(p, tOp.z, CMPP_RC_SYNTAX,
                     "Infix 'not' must be followed by 'glob' or 'in'.");
        break;
      }
    }
    int const lbp = cmpp__xp_lbp(op);
    if( !lbp ){
      cmpp__xp_err(p, tOp.z, CMPP_RC_SYNTAX,
                   "Expecting a binary operator but got: %.*s",
                   (int)tOp.n, tOp.z);
      break;
    }
    if( lbp<=minBp ){
      if( XOp_notGlob==op || XOp_notIn==op ){
        /* Un-read the "glob" or "in". */
        *p->tz = tzSave;
        p->tok = tOp;
        p->op = XOp_not;
      }
      break;
    }
    if( XOp_in==op || XOp_notIn==op ){
      /* Reserved. The RHS will need to be resolved as a list, which
         is what cmpp__expr_operand_LIST is reserved for. */
      cmpp__xp_err(p, tOp.z, CMPP_RC_UNSUPPORTED,
                   "The 'in' operator is reserved but not yet "
                   "implemented.");
      break;
    }
    if( cmpp__xp_next(p) ) break;
    bool shortCircuit = false;
    if( !p->skip && (XOp_and==op || XOp_or==op) ){
      bool const t = cmpp__xval_truth(rv);
      shortCircuit = XOp_and==op ? !t : t;
      cmpp__xval_set_int(pp, rv, t);
    }
    if( shortCircuit ) ++p->skip;
    cmpp__xp_expr(p, lbp, &rhs);
    if( shortCircuit ) --p->skip;
    if( xpCode || p->skip || shortCircuit ){
      cmpp__xval_clear(pp, &rhs);
      continue;
    }
    switch( op ){
      case XOp_and:
      case XOp_or:
        /* Not short-circuited, so the result is the RHS's truth. */
        cmpp__xval_set_int(pp, rv, cmpp__xval_truth(&rhs));
        break;
      case XOp_eq: case XOp_ne:
      case XOp_lt: case XOp_le: case XOp_gt: case XOp_ge:
        cmpp__xp_compare(p, op, rv, &rhs);
        break;
      case XOp_glob:
      case XOp_notGlob:{
        char bufL[24], bufR[24];
        unsigned char const *zL, *zR;
        cmpp_size_t nL, nR;
        cmpp_b * const bL = cmpp_b_borrow(pp);
        cmpp_b * const bR = cmpp_b_borrow(pp);
        if( bL && bR ){
          /* sqlite3_strglob() needs NUL-terminated inputs. */
          cmpp__xval_bytes(rv, bufL, &zL, &nL);
          cmpp__xval_bytes(&rhs, bufR, &zR, &nR);
          if( 0==cmpp_b_append4(pp, bL, zL, nL)
              && 0==cmpp_b_append4(pp, bR, zR, nR) ){
            bool m = 0==sqlite3_strglob(
              bR->z ? (char const *)bR->z : "",
              bL->z ? (char const *)bL->z : "");
            if( XOp_notGlob==op ) m = !m;
            cmpp__xval_set_int(pp, rv, m);
          }
        }
        cmpp_b_return(pp, bL);
        cmpp_b_return(pp, bR);
        break;
      }
      default:
        cmpp__xp_arith(p, op, &tOp, rv, &rhs);
        break;
    }
    cmpp__xval_clear(pp, &rhs);
  }
  cmpp__xval_clear(pp, &rhs);
  return xpCode;
}
/* …（同文件无关代码省略）… */
int cmpp__expr_eval(cmpp__expr const * ex,
                    unsigned char const * z, cmpp_size_t n,
                    cmpp__xval * rv){
  cmpp * const pp = ex->pp;
  cmpp_tizer tz = cmpp_tizer_empty;
  CmppXP xp = {
    .ex = ex, .pp = pp, .tz = &tz,
    .tok = cmpp_token_empty_m, .op = 0,
    .skip = (cmpp__expr_F_PARSE_ONLY & ex->flags) ? 1 : 0
  };
  CmppXP * const p = &xp /* for the xpCode macro */;
  if( xpCode ) return xpCode;
  cmpp__xval_clear(pp, rv);
  cmpp_tizer const * const tzParent = ex->tzParent;
  cmpp_tizer_init(&tz, tzParent ? tzParent->err : &pp->pimpl->err,
                  z, (cmpp_strlen_t)n);
  if( tzParent ){
    tz.name = cmpp_tizer_name(tzParent);
    if( cmpp_tizer_contains(tzParent, z) ){
      /* Lets error reporting calculate line numbers relative to the
         parent's input. */
      tz.parent = tzParent;
    }
  }
  if( 0==cmpp__xp_next(p) ){
    if( cmpp_tt_EOF==p->tok.ttype ){
      cmpp__xp_err(p, z, CMPP_RC_SYNTAX,
                   "An empty expression is not permitted.");
    }else{
      cmpp__xp_expr(p, BP_NONE, rv);
      /* cmpp__xp_expr() only stops early on error or EOF. */
      assert( xpCode || cmpp_tt_EOF==p->tok.ttype );
    }
  }
  cmpp_tizer_dtor(&tz);
  int const rc = xpCode;
  if( rc ) cmpp__xval_clear(pp, rv);
  return rc;
}
/* …（同文件无关代码省略）… */
int cmpp__expr_eval_core(cmpp *pp, unsigned char const * z,
                         cmpp_size_t n, cmpp__xval * rv){
  cmpp__expr const ex = {
    .pp = pp, .xOperand = cmpp__expr_operand_core, .state = 0,
    .flags = 0, .tzParent = 0, .xErr = 0, .xErrCode = 0, .xTzErr = 0
  };
  return cmpp__expr_eval(&ex, z, n, rv);
}
/* …（同文件无关代码省略）… */
int cmpp__expr_eval_int(cmpp *pp, unsigned char const * z,
                        cmpp_size_t n, int64_t * pOut){
  cmpp__xval v = cmpp__xval_empty;
  if( 0==cmpp__expr_eval_core(pp, z, n, &v) ){
    cmpp__xval_to_int(pp, &v, pOut);
  }
  cmpp__xval_clear(pp, &v);
  return ppCode;
}
/* …（同文件无关代码省略）… */
int cmpp__expr_eval_paren(cmpp *pp, cmpp_arg const * arg,
                          int64_t * pOut){
  assert( cmpp_tt_GroupParen==arg->ttype );
  /* Prefer the token's range, which points back into the source (so
     errors can report their location), over arg->z, which may be a
     copy. */
  return arg->token.zInner
    ? cmpp__expr_eval_int(pp, arg->token.zInner, arg->token.nInner, pOut)
    : cmpp__expr_eval_int(pp, arg->z, arg->n, pOut);
}

#undef xpCode


#if CMPP_ENABLE_DLLS && !defined(CMPP_OMIT_D_MODULE)
/* …（同文件无关代码省略）… */
#  define SQLITE_EXTENSION_INIT2(X) (void)X
/* …（同文件无关代码省略）… */
int sqlite3_series_init(
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);
#ifndef SQLITE_OMIT_VIRTUALTABLE
  if( sqlite3_libversion_number()<3008012 && pzErrMsg!=0 ){
    *pzErrMsg = sqlite3_mprintf(
        "generate_series() requires SQLite 3.8.12 or later");
    return SQLITE_ERROR;
  }
  rc = sqlite3_create_module(db, "generate_series", &seriesModule, 0);
#endif
  return rc;
}
/* …（同文件无关代码省略）… */
char const * cmpp__tt_cstr2(int tt, bool bSymbolName){
  switch(tt){
#define E(N,V,M) case cmpp_tt_ ## N:        \
    return bSymbolName ? "cmpp_tt_" # N : # N;
    cmpp_tt_map(E)
#undef E
  }
  return NULL;
}
/* …（同文件无关代码省略）… */
static bool cmpp_tizer_terminates_group(int groupType, int ch){
  switch(ch){
    case 0:
    case ' ': case '\n': case '\r': case '\t': case ';': return true;
    case ']': case ')': case '}':
      /* experiment 2026-08-31 to get {a b !!} to work */
      //return cmpp_tt_shape_string==(cmpp_tt_mask_shape & groupType);
      (void)groupType;
      return true;
    default: return false;
  }
}
/* …（同文件无关代码省略）… */
      case '"': case '`': case '\'':{
        if( tok->ttype || zEsc ){
          /* Scratch that - this is a continuation */
          goto do_word;
        }
        //g_warn("*z=%c isEsc=%d", *z, (int)!!zEsc);
/* …（同文件无关代码省略）… */
                   (int)tok->nInner, tok->zInner);
          }
          if( z<zEnd ){
            if( !cmpp_tizer_terminates_group(tty, *z) ){
              /* We're eventually going to want to support a Tcl-like
                 bellybutton operation, in which case this has to be
                 patched. Or maybe we'll use JS syntax for that
/* …（同文件无关代码省略）… */
#if defined(CMPP_MAIN_AUTOLOADER)
  {
    extern int CMPP_MAIN_AUTOLOADER(cmpp*,char const *,void*);
    cmpp_d_autoloader al = cmpp_d_autoloader_empty;
    al.f = CMPP_MAIN_AUTOLOADER;
    cmpp_d_autoloader_set(pp, &al);
  }
