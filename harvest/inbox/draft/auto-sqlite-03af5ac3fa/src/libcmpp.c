// AUTO-DRAFT from sqlite/sqlite PR #1a16a44e9598c8757c05f125b4ae0061a61810bf
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
#define CMPP_LIB_VERSION "2.0.x"
#define CMPP_LIB_VERSION_HASH "4539e17f451054d2aeb6a5ffe3891af087fd8fa6cfae4e31d02e1fddfec69d46"
#define CMPP_LIB_VERSION_TIMESTAMP "2026-09-30 09:12:44.230 UTC"
/* …（同文件无关代码省略）… */
#define CMPP_VERSION CMPP_LIB_VERSION " " CMPP_LIB_VERSION_HASH " @ " CMPP_LIB_VERSION_TIMESTAMP
/* …（同文件无关代码省略）… */
#  define CMPP_EXPORT extern
/* …（同文件无关代码省略）… */
  typedef void cmpp_FILE;
/* …（同文件无关代码省略）… */
typedef struct cmpp__pimpl cmpp__pimpl;
/* …（同文件无关代码省略）… */
typedef struct cmpp_outputer cmpp_outputer;
typedef struct cmpp_b cmpp_b;
typedef struct cmpp_arg cmpp_arg;
/* …（同文件无关代码省略）… */
   it is invoked once for each entry and passed 3 arguments: the enum
   entry's full name (CMPP_RC_...), its integer value, and a terse
   informational string.
*/
#define cmpp_rc_e_map(E) \
  E(CMPP_RC_OK, 0,                                                      \
/* …（同文件无关代码省略）… */
   for CMPP_RC_OK, which is guaranteed to always be 0 (and the API
   guarantees that no other code shall have a value of zero).

   The only reasons numbers are hard-coded to the values is to
   simplify debugging during development. Clients may use
   cmpp_rc_cstr() to get some human-readable (or programmer-readable)
   form for any given value in this enum.
*/
enum cmpp_rc_e {
#define E(N,V,H) N = V,
/* …（同文件无关代码省略）… */
   it's not a member of that enum.
*/
char const * cmpp_rc_cstr(int rc);
#define cmpp_rc_ucstr(RC) (unsigned char const*)cmpp_rc_cstr(RC)

/**
   CMPP_BITNESS specifies whether the library should use 32- or 64-bit
/* …（同文件无关代码省略）… */
typedef uint32_t cmpp_size_t;
/* …（同文件无关代码省略）… */

  Maintenance reminder: this type must remain 32-bits or risks
  breaking countless printf-style formatting uses, where it frequently
  ends up being used with "%.*s".
*/
typedef int32_t cmpp_strlen_t;

/* …（同文件无关代码省略）… */
#define CMPP_SIZE_T_PFMT PRIu32
/* …（同文件无关代码省略）… */
typedef struct cmpp cmpp;
/* …（同文件无关代码省略）… */
CMPP_EXPORT int cmpp_undefine(cmpp *pp, const char * zKey,
                              cmpp_strlen_t n);

/**
   This works similarly to cmpp_define_v2() except that the new define
   "shadows", rather than overwrites, an existing define with the same
/* …（同文件无关代码省略）… */
#define cmpp_err_c(PP,RC,MSG) cmpp_err(PP, RC, "" MSG, sizeof(MSG)-1)
/* …（同文件无关代码省略）… */
typedef struct cmpp_errinfo cmpp_errinfo;
/* …（同文件无关代码省略）… */
};

/**
   cmpp token types, currently distinct from the public API's cmpp_tt_map
   as we consolidate the two.

   The first argument to E is the tail of the cmpp_tt_ ## TAIL part
   of the enum entry's name.  The second is the cmpp_tt_mask_id part
/* …（同文件无关代码省略）… */
#define cmpp_tt_map(E)                        \
  E(None,          0,   0)                      \
  E(Error,         1,   cmpp_tt_shape_flow)   \
  /** Anything with no more specific type. */   \
  E(Word,          2,   cmpp_tt_shape_word)   \
  E(StringSQ,      3,   cmpp_tt_shape_string) \
  E(StringDQ,      4,   cmpp_tt_shape_string) \
  /** Backtick string. */                       \
  E(StringBT,      5,   cmpp_tt_shape_string) \
  /** Decimal int, optionally signed but some
/* …（同文件无关代码省略）… */
typedef struct cmpp_token cmpp_token;
/* …（同文件无关代码省略）… */
char const * cmpp_token_cstr3(cmpp_token const * t, cmpp_size_t * n, bool inner);

int cmpp_token_strcmp(cmpp_token const * t, char const *z, cmpp_strlen_t n);
#define cmpp_token_ustrcmp(T,Z,N) \
  cmpp_token_strcmp((T),(unsigned char const *)(Z),(N))

/** Convenience form of cmpp_token_cstr3() which casts the result to
    (unsigned char const *). */
#define cmpp_token_ucstr3(T,NP,I) \
  (unsigned char const *)cmpp_token_cstr3((T),(NP),(I))

/**
/* …（同文件无关代码省略）… */
typedef struct cmpp_dx cmpp_dx;
typedef struct cmpp__dx_pimpl cmpp__dx_pimpl;
/* …（同文件无关代码省略）… */
*/
CMPP_EXPORT char const * cmpp_arg_cstr2(cmpp_arg const *arg, cmpp_size_t *n);

#define cmpp_arg_ucstr(ARG) (unsigned char const*)cmpp_arg_cstr(ARG)
#define cmpp_arg_ucstr2(ARG,N) (unsigned char const*)cmpp_arg_cstr2((ARG),(N))

/**
   This is the core argument-parsing function used by the library's
/* …（同文件无关代码省略）… */
typedef struct cmpp_dx_pos cmpp_dx_pos;
/* …（同文件无关代码省略）… */
   Returns, via its output parameters, the current name and input line
   number of dx's current directive. This is intended for use with
   synthesizing names such as those of embedded scripts. The *zName
   bytes are owned by dx and are valid until it is destroyed.

   Caveat: this specifically refers to the line holding the current
   directive, not any non-directive lines consumed since that
   directive was captured.
*/
CMPP_EXPORT void cmpp_dx_src_pos_info(cmpp_dx const *dx, char const **zName,
                                      cmpp_size_t * lineNo);
/* …（同文件无关代码省略）… */
                                 cmpp_strlen_t n, int flags);


/**
   Expects [z,z+n) to point to a single JSON value. If
   cmpp_json_valid() returns true for the range, it is appended to be
   as-is, else it is appended as a JSON-quoted string. Returns 0 on
   success, non-0 (OOM or related range overflow) on error, updating
   pp's error state if it returns non-0.

   For cmpp_json_valid() purposes, a flags value of 3 (JSON/JSON5) is
   used.
*/
CMPP_EXPORT int cmpp_b_append_json_value(cmpp *pp, cmpp_b *b,
                                         unsigned char const *z,
                                         cmpp_strlen_t n);


/**
   Returns true if [z,z+n) begins with '[', ends with ']', and
   cmpp_json_valid() for the same range returns true. i.e. if it's a
/* …（同文件无关代码省略）… */
CMPP_EXPORT bool cmpp_json_is_object(cmpp *pp, unsigned char const *z,
                                     cmpp_strlen_t n, bool allowJson5);

/**
   Appends some form of [z,z+n) to str, using strlen() to calculate n
   if n is negative:
/* …（同文件无关代码省略）… */
   CMPP_RC_... code, because it's more useful that way (but also
   slightly confusing).
*/
CMPP_EXPORT int cmpp_stmt_step(cmpp * const pp, sqlite3_stmt * const q,
                               bool resetIt);

/**
/* …（同文件无关代码省略）… */
   already set then pp's error state is updated and a non-0 value is
   returned.
*/
CMPP_EXPORT int cmpp_stmt_reset(cmpp *pp, sqlite3_stmt * const q);

/**
   Expects an SQLite result value. If it's SQLITE_OK, SQLITE_ROW, or
/* …（同文件无关代码省略）… */
#define cmpp_db_rc(PP,RC,ZMSG) cmpp_db_rc_v2(PP, RC, ZMSG, -1)
/* …（同文件无关代码省略）… */
                                     cmpp_finalizer_f finalizer);




/**
   The current cmpp_api_thunk::apiVersion value.  See
   cmpp_api_thunk_map.
*/
#define cmpp_api_thunk_version 20260929

/**
   This stub object is provided for cmpp interface compatibility with
/* …（同文件无关代码省略）… */
  uint32_t const thunkVersion;
};

#ifdef __cplusplus
} /* extern "C" */
#endif
/* …（同文件无关代码省略）… */
#define CMPP__DB_MAIN_NAME "cmpp"
/* …（同文件无关代码省略）… */
#define CMPP_PRIVATE static
/* …（同文件无关代码省略）… */
  typedef struct T T;                                  \
/* …（同文件无关代码省略）… */

#define cmpp__ListType_impl(T,MT) \
  int T ## _reserve(cmpp *pp,struct T *li, cmpp_size_t min) { \
    return cmpp_array_reserve(pp, (void**)&li->list, min,     \
                              &li->na, sizeof(MT));           \
  }

#define cmpp__LIST_T_empty_m {.list=0,.n=0,.na=0}
/* …（同文件无关代码省略）… */
typedef struct CmppDLine CmppDLine;
/* …（同文件无关代码省略）… */
  */
  cmpp_dx_pos pos;
  /**
     Currently input line.
  */
  CmppDLine dline;
  /** Number of active #savepoints. */
  unsigned nSavepoint;
  /** Current directive's args. Updated by the parsing and dispatching
      steps. */
/* …（同文件无关代码省略）… */
  CmppLvlList dxLvl;
  struct {
    /**
       A copy of this->d's input line which gets translated
       slightly from its native form for futher processing.
    */
    cmpp_b line;
    /**
/* …（同文件无关代码省略）… */
#define cmpp__epol(PP,WHICH) (PP)->pimpl->policy.WHICH
#define cmpp__policy(PP,WHICH) \
  cmpp__epol(PP,WHICH).stack[cmpp__epol(PP,WHICH).n]
/* …（同文件无关代码省略）… */
#define CMPP__SEL_V_FROM(N)            \
    "(SELECT v FROM " CMPP__DB_MAIN_NAME ".vdef WHERE k=?" #N \
    " ORDER BY source LIMIT 1)"
/* …（同文件无关代码省略）… */
#define CMPP__SAVEPOINT_NAME "_cmpp_"
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
    E(defGetBool,                                      \
      "SELECT cmpp_truthy(v) FROM "                    \
      CMPP__DB_MAIN_NAME ".vdef"                       \
      " WHERE k = ?1"                                  \
      " ORDER BY source LIMIT 1")                      \
    E(defGetInt,                                       \
      "SELECT CAST(v AS INTEGER)"                      \
      " FROM " CMPP__DB_MAIN_NAME ".vdef"              \
      " WHERE k = ?1"                                  \
      " ORDER BY source LIMIT 1")                      \
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
    /* Compare two values. */                          \
    E(cmpVV, "SELECT cmpp_compare(?1,?2)")             \
    /* Compare LHS define to value ?2. */              \
    E(cmpDV,                                           \
      "SELECT cmpp_compare("                           \
      CMPP__SEL_V_FROM(1) ", ?2"                       \
      ")")                                             \
    /* Compare value ?1 to RHS define. */              \
    E(cmpVD,                                           \
      "SELECT cmpp_compare("                           \
      "?1," CMPP__SEL_V_FROM(2)                        \
      ")")                                             \
    /* Compare defines ?1 and ?2. */                   \
    E(cmpDD,                                           \
      "SELECT cmpp_compare("                           \
      CMPP__SEL_V_FROM(1)                              \
      ","                                              \
      CMPP__SEL_V_FROM(2)                              \
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
    E(jsonValid,                                       \
      /* beware of validation of json_valid() ?2 */    \
      "SELECT json_valid(?1,?2)")                      \

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
static inline void cmpp__err_reuse(cmpp *pp){
  cmpp_errinfo_reuse(&pp->pimpl->err);
}
/* …（同文件无关代码省略）… */
   the unresolved key.

   Wonky return semantics.
*/
CMPP_PRIVATE int cmpp__get_b(cmpp *pp, unsigned const char * zName,
                             cmpp_strlen_t nName, cmpp_b * b,
/* …（同文件无关代码省略）… */
static inline cmpp_size_t cmpp__strlen(char const *z, cmpp_strlen_t n){
  return n<0 ? (cmpp_size_t)strlen(z) : (cmpp_size_t)n;
}
static inline cmpp_size_t cmpp__strlenu(unsigned char const *z, cmpp_strlen_t n){
  return n<0 ? (cmpp_size_t)strlen((char const *)z) : (cmpp_size_t)n;
}
/* …（同文件无关代码省略）… */
#define cmpp__fatal(...) cmpp__fatal_base(__FILE__,__LINE__,__VA_ARGS__)
/* …（同文件无关代码省略）… */
#define g_warn(zFmt,...) g_stderr("%s:%d %s() " zFmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
/* …（同文件无关代码省略）… */
#define g_debug(PP,lvl,pfexpr) (void)0
/* …（同文件无关代码省略）… */
#define ustr_c(X) ((unsigned char const *)X)
/* …（同文件无关代码省略）… */
#define cmpp__pi(PP) cmpp__pimpl * const pi = PP->pimpl
#define cmpp__dx_pi(DX) cmpp__dx_pimpl * const dpi = DX->pimpl
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
  } zLeft = z
  for( ; z<zEnd && 0==ppCode; ++z ){
    zLeft = z;
    for( ;z<zEnd && 0==ppCode; ++z ){
    again:
      if( chEol==*z ){
        state = state_opening;
        continue;
/* …（同文件无关代码省略）… */
}

int cmpp__get_b(cmpp *pp, unsigned const char * zName,
                cmpp_strlen_t nName, cmpp_b * os, bool enforceUndefPolicy){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defGet, false);
  if( q ){
    nName = cmpp__strlenu(zName, nName);
    cmpp_stmt_bind_textx(pp, q, 1, zName, nName, SQLITE_STATIC);
    int n = 0;
    if(SQLITE_ROW == cmpp_stmt_step(pp, q, false)){
      const unsigned char * z = sqlite3_column_text(q, 3);
      n = sqlite3_column_bytes(q, 3);
      cmpp_b_append4(pp, os, z, (cmpp_size_t)n);
      rc = 1;
    }else{
      if( enforceUndefPolicy ){
        cmpp__affirm_undef_policy(pp, zName, nName);
      }
      rc = 0;
    }
    cmpp_stmt_reset(pp, q);
    g_debug(pp,1,("get-define [%.*s] ?= %d %.*s\n",
                  nName, zName, rc, os->n, os->z));
  }
  return rc;
}

#if 0
int cmpp__get_b2(cmpp *pp, unsigned const char * zName,
                 cmpp_strlen_t nName, cmpp_b * os, bool enforceUndefPolicy,
                 bool *gotOne){
  static bool nope;
  *(gotOne ? gotOne : &nope) = cmpp__get_b(pp, zName, nName, os,
                                           enforceUndefPolicy);
  return ppCode;
}
#endif

CMPP__EXPORT(int, cmpp_undef_legacy)(cmpp *pp, const char * zKey,
                                     unsigned int *nRemoved){
/* …（同文件无关代码省略）… */
         are equivalent.  The relavent code is in another tree,
         awaiting a decision on whether to import it or re-base cmpp
         on top of that library (which would, e.g., replace cmpp_b
         with that one, which is more mature).
      */
      cmpp_stmt_bind_int(pp, q, 1, priority);
      cmpp_stmt_bind_textx(pp, q, 2, ustr_c(zDir), -1, SQLITE_STATIC);
/* …（同文件无关代码省略）… */
int cmpp_array_reserve(cmpp *pp, void **list, cmpp_size_t nDesired,
                       cmpp_size_t * nAlloc, unsigned sizeOfEntry){
  int rc = pp ? ppCode : 0;
  if( 0==rc && nDesired > *nAlloc ){
    cmpp_size_t const nA = nDesired < 10 ? 10 : nDesired;
    void * const p = cmpp_mrealloc(*list, sizeOfEntry * nA);
    rc = cmpp_check_oom(pp, p);
    if( p ){
      memset((unsigned char *)p +
             (sizeOfEntry * *nAlloc), 0,
             sizeOfEntry * (nA - *nAlloc));
      *list = p;
      *nAlloc = nA;
    }
  }
  return rc;
}
/* …（同文件无关代码省略）… */
void cmpp__dx_input_range(cmpp_dx const * dx, unsigned char const **zBegin,
                          unsigned char const **zEnd){
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
  *zBegin = dx->pimpl->zBegin;
  *zEnd = dx->pimpl->zEnd;
}
/* …（同文件无关代码省略）… */
#define out(CH) assert(zo<zoEnd); (*zo++)=CH
/* …（同文件无关代码省略）… */
  assert( dx->sourceName );
  enum { BufSize = 512 * 4 };
  unsigned char buf[BufSize] = {0};
  unsigned char *z = &buf[0];
  cmpp_size_t n = cmpp__strlenu(dx->sourceName, -1);
  if( n > (unsigned)BufSize-1 ) return;
  memcpy(z, dx->sourceName, n);
  buf[n] = 0;
  cmpp_strlen_t i = n - 1;
  for( ; i > 0; --i ){
    if( '/'==z[i] || '\\'==z[i] ){
      z[i] = 0;
      n = i;
      break;
    }
/* …（同文件无关代码省略）… */
}

int cmpp_output_f_fd( void * state, void const * src, cmpp_size_t n ){
  int const fd = *((int*)state);
  ssize_t const wn = write(fd, src, n);
  return wn<0 ? cmpp_errno_rc(errno, CMPP_RC_IO) : 0;
}

int cmpp_input_f_FILE( void * state, void * dest, cmpp_size_t * n ){
  cmpp_FILE * f = state;
  cmpp_size_t const rn = *n;
  *n = (cmpp_size_t)fread(dest, 1, rn, f);
  return *n==rn ? 0 : (feof(f) ? 0 : CMPP_RC_IO);
}

int cmpp_input_f_fd( void * state, void * dest, cmpp_size_t * n ){
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
  unsigned char const *zB = 0;
  unsigned char const *zE = 0;
  unsigned char const *zErr = dpi->zErrPos;
  cmpp__dx_input_range(dx, &zB, &zE);
  //g_warn("Whole script:\n%.*s", (int)(zE-zB), zB);
  assert( zB );
  assert( zE );
  assert( zB<=zE );
  if( !zErr ){
    if( dpi->pos.z>=zB && dpi->pos.z<zE ){
      /* This is frequently past zEnd */
      zErr = dpi->pos.z;
    }else{
      zErr = dpi->dline.zBegin;
    }
  }
  //g_warn("dpi->zErrPos: %p\n%.3s ...", dpi->zErrPos, dpi->zErrPos);
  //g_warn("dpi->pos.z: %p\n %.3s ...", dpi->pos.z, dpi->pos.z);
  //g_warn("zErr: %p\n%.3s ...", zErr, zErr);
  assert( (!zErr || (zErr>=zB && zErr<=zE))
          && "Else we're pointing to incompatible sources");
  cmpp_size_t const nl = zErr
    ? 1 + cmpp_count_nl(zB, zE, zErr, NULL)
    : dpi->dline.lineNo;
  if( dx->d ){
    sqlite3_str_appendf(sstr,  "%s @ %s:%d",
                        dx->d->name.z,
                        (dx->sourceName
                         && 0==strcmp("-", (char const *)dx->sourceName))
                        ? "<stdin>"
                        : (char const *)dx->sourceName,
                        (int)dpi->dline.lineNo);
  }
  if( nl != dpi->dline.lineNo ){
    /* Input position past the directive line. */
    sqlite3_str_appendf(sstr, " from line %" CMPP_SIZE_T_PFMT, nl);
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
/* …（同文件无关代码省略）… */
  return cmpp__json_is_container(pp, '{', '}', z, n, allowJson5);
}

CMPP__EXPORT(void,cmpp_str_json_append)(cmpp *pp, sqlite3_str * str,
                                        unsigned char const * z,
                                        cmpp_strlen_t n){
/* …（同文件无关代码省略）… */
  }
}

void cmpp__dump_defines(cmpp *pp, cmpp_FILE * fp, int bIndent){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defSelAll, false);
  if( q ){
/* …（同文件无关代码省略）… */
}


/**
   This is what was originally the main() of cmpp v1, back when it was
   a monolithic app. It still serves as the driver for main() but is
/* …（同文件无关代码省略）… */
  if( eL==eR ) return 0;
  else if( !eL ) return 1;
  else if (!eR ) return -1;
  return (int)(/*largest first*/eL->nAlloc - eR->nAlloc);
}

/**
/* …（同文件无关代码省略）… */
  if( eL==eR ) return 0;
  else if( !eL ) return 1;
  else if (!eR ) return -1;
  return (int)(/*smallest first*/eR->nAlloc - eL->nAlloc);
}

/**
/* …（同文件无关代码省略）… */
#define PC 0x80 /* pad character */
#define WS 0x81 /* whitespace */
/* …（同文件无关代码省略）… */
    : (b->n<nn ? -1 : 1/* prefix match: shortest first */);
}

CMPP__EXPORT(int,cmpp_b_append_json_value)(cmpp *pp, cmpp_b *b,
                                           unsigned char const *z,
                                           cmpp_strlen_t n){
  int64_t j = 0;
  int rc;
  n = cmpp__strlenu(z, n);
  if( cmpp_is_int64(z, n, &j) ){
    rc = cmpp_b_append_i64(b, j);
    if( rc ) cmpp_err(pp, rc, 0, 0);
  }else if( cmpp_json_valid(pp, z, n, 3) ){
    rc = cmpp_b_append4(pp, b, z, n);
  }else {
    sqlite3_str * const s = cmpp__sqlite3_str_new(pp);
    if( s ){
      sqlite3_str_appendf(s, "%J", z);
      int nn = 0;
      char * const zz = cmpp_sqlite3_str_finish(pp, s, &nn);
      if( zz ){
        rc = cmpp_b_append4(pp, b, zz, n);
      }else{
        rc = cmpp_err_has(pp);
        assert( rc );
      }
      cmpp_mfree(zz);
    }else{
      rc = cmpp_err_has(pp);
      assert( rc );
    }
  }
  return rc;
}

#undef PC
#undef WS
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
  return rc;
}

static inline int cmpp__stmt_is_sp(cmpp const * const pp,
                                   sqlite3_stmt const * const q){
  return q==pp->pimpl->stmt.spBegin
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
**
*/
#if 0
#include "sqlite3ext.h"
#else
#include "sqlite3.h"
#endif
#include <assert.h>
#include <string.h>
/* …（同文件无关代码省略）… */
typedef unsigned char u8;
typedef struct series_cursor series_cursor;
/* …（同文件无关代码省略）… */
  sqlite3_int64 iValue;      /* Current value */
  u8 bDesc;                  /* iStep is really negative */
  u8 bDone;                  /* True if stepped past last element */
};

/*
/* …（同文件无关代码省略）… */
static sqlite3_int64 add64(sqlite3_int64 a, sqlite3_uint64 b){
  sqlite3_uint64 x = *(sqlite3_uint64*)&a;
  x += b;
  return *(sqlite3_int64*)&x;
}
static sqlite3_int64 sub64(sqlite3_int64 a, sqlite3_uint64 b){
  sqlite3_uint64 x = *(sqlite3_uint64*)&a;
  x -= b;
  return *(sqlite3_int64*)&x;
}
/* …（同文件无关代码省略）… */
  rc = sqlite3_declare_vtab(db,
     "CREATE TABLE x(value,start hidden,stop hidden,step hidden)");
  if( rc==SQLITE_OK ){
    pNew = *ppVtab = sqlite3_malloc( sizeof(*pNew) );
    if( pNew==0 ) return SQLITE_NOMEM;
    memset(pNew, 0, sizeof(*pNew));
    sqlite3_vtab_config(db, SQLITE_VTAB_INNOCUOUS);
/* …（同文件无关代码省略）… */
static int seriesOpen(sqlite3_vtab *pUnused, sqlite3_vtab_cursor **ppCursor){
  series_cursor *pCur;
  (void)pUnused;
  pCur = sqlite3_malloc( sizeof(*pCur) );
  if( pCur==0 ) return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));
  *ppCursor = &pCur->base;
/* …（同文件无关代码省略）… */
    pCur->bDone = 1;
  }else if( pCur->bDesc ){
    pCur->iValue = sub64(pCur->iValue, pCur->iStep);
    assert( pCur->iValue>=pCur->iTerm );
  }else{
    pCur->iValue = add64(pCur->iValue, pCur->iStep);
    assert( pCur->iValue<=pCur->iTerm );
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
#define LARGEST_INT64  ((sqlite3_int64)0x7fffffffffffffffLL)
/* …（同文件无关代码省略）… */
#define SMALLEST_INT64 ((sqlite3_int64)0x8000000000000000LL)
/* …（同文件无关代码省略）… */
static double seriesCeil(double r){ return ceil(r); }
static double seriesFloor(double r){ return floor(r); }
/* …（同文件无关代码省略）… */
}
#endif

/*
** This method is called to "rewind" the series_cursor object back
** to the first row of output.  This method is always called at least
/* …（同文件无关代码省略）… */
    if( idxNum & 0x40 ){
      iOffset = sqlite3_value_int64(argv[iArg++]);
    }
  }

  /* Narrow the range of iMin and iMax (the minimum and maximum outputs)
  ** based on equality and inequality constraints on the "value" column.
  */
  if( idxNum & 0x3380 ){
    if( idxNum & 0x0080 ){    /* value=X */
      if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
        double r = sqlite3_value_double(argv[iArg++]);
        if( r==seriesCeil(r)
         && r>=(double)SMALLEST_INT64
         && r<=(double)LARGEST_INT64
        ){
          iMin = iMax = (sqlite3_int64)r;
        }else{
          goto series_no_rows;
        }
      }else{
        iMin = iMax = sqlite3_value_int64(argv[iArg++]);
      }
    }else{
      if( idxNum & 0x0300 ){  /* value>X or value>=X */
        if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
          double r = sqlite3_value_double(argv[iArg++]);
          if( r<(double)SMALLEST_INT64 ){
            iMin = SMALLEST_INT64;
          }else if( (idxNum & 0x0200)!=0 && r==seriesCeil(r) ){
            iMin = (sqlite3_int64)seriesCeil(r+1.0);
          }else{
            iMin = (sqlite3_int64)seriesCeil(r);
          }
        }else{
          iMin = sqlite3_value_int64(argv[iArg++]);
/* …（同文件无关代码省略）… */
          }
        }
      }
      if( idxNum & 0x3000 ){   /* value<X or value<=X */
        if( sqlite3_value_numeric_type(argv[iArg])==SQLITE_FLOAT ){
          double r = sqlite3_value_double(argv[iArg++]);
          if( r>(double)LARGEST_INT64 ){
            iMax = LARGEST_INT64;
          }else if( (idxNum & 0x2000)!=0 && r==seriesFloor(r) ){
            iMax = (sqlite3_int64)(r-1.0);
          }else{
            iMax = (sqlite3_int64)seriesFloor(r);
          }
        }else{
          iMax = sqlite3_value_int64(argv[iArg++]);
/* …（同文件无关代码省略）… */
      aIdx[iCol] = i;
    }
  }
  if( aIdx[3]==0 ){
    /* Ignore OFFSET if LIMIT is omitted */
    idxNum &= ~0x60;
/* …（同文件无关代码省略）… */
    ** the preferred case */
    pIdxInfo->estimatedCost = (double)(2 - ((idxNum&4)!=0));
    pIdxInfo->estimatedRows = 1000;
    if( pIdxInfo->nOrderBy>=1 && pIdxInfo->aOrderBy[0].iColumn==0 ){
      if( pIdxInfo->aOrderBy[0].desc ){
        idxNum |= 0x08;
      }else{
        idxNum |= 0x10;
      }
      pIdxInfo->orderByConsumed = 1;
    }
  }else if( (idxNum & 0x21)==0x21 ){
    /* We have start= and LIMIT */
    pIdxInfo->estimatedRows = 2500;
/* …（同文件无关代码省略）… */
int sqlite3_series_init(
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;
  (void)pApi;
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
