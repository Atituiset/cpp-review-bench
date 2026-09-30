// AUTO-DRAFT from sqlite/sqlite PR #226a2c0d6ad3f7fb5a291b98c7093b0ce5aba163
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

  with libcmpp 2.0.x e668c06b80424adfc4e2a19a30fa36d1c91071a4a573234336dfab08d29f6add @ 2026-09-15 07:14:55.343 UTC
*/
#if !defined(NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED)
#define NET_WANDERINGHORSE_LIBCMPP_C_INCLUDED
/* …（同文件无关代码省略）… */

  ./c-pp -I. -I./src -Dsrcdir=./src -o libcmpp.h ./tool/libcmpp.c-pp.h

  with libcmpp 2.0.x e668c06b80424adfc4e2a19a30fa36d1c91071a4a573234336dfab08d29f6add @ 2026-09-15 07:14:55.343 UTC
*/
#define CMPP_PACKAGE_NAME "libcmpp"
#define CMPP_LIB_VERSION "2.0.x"
#define CMPP_LIB_VERSION_HASH "e668c06b80424adfc4e2a19a30fa36d1c91071a4a573234336dfab08d29f6add"
#define CMPP_LIB_VERSION_TIMESTAMP "2026-09-15 07:14:55.343 UTC"
#define CMPP_LIB_CONFIG_TIMESTAMP "2026-09-15 13:06 GMT"
#define CMPP_VERSION CMPP_LIB_VERSION " " CMPP_LIB_VERSION_HASH " @ " CMPP_LIB_VERSION_TIMESTAMP
#define CMPP_PLATFORM_EXT_DLL ".so"
#define CMPP_MODULE_PATH ".:/usr/local/lib/cmpp"
/* …（同文件无关代码省略）… */
#  define CMPP_EXPORT extern
/* …（同文件无关代码省略）… */
  typedef void cmpp_FILE;
/* …（同文件无关代码省略）… */
#include <stdbool.h>
#include "sqlite3.h" /* sqlite3_str */

typedef struct cmpp_arg cmpp_arg;

/**
   For loadable modules to be able to portably access the cmpp API,
   without requiring that their loading binary be linked with
/* …（同文件无关代码省略）… */
typedef struct cmpp__pimpl cmpp__pimpl;
typedef struct cmpp_api_thunk cmpp_api_thunk;
typedef struct cmpp_outputer cmpp_outputer;
typedef void cmpp_itch;

/**
/* …（同文件无关代码省略）… */
typedef uint32_t cmpp_flag32_t;
/* …（同文件无关代码省略）… */
   it's not a member of that enum.
*/
char const * cmpp_rc_cstr(int rc);

/**
   CMPP_BITNESS specifies whether the library should use 32- or 64-bit
/* …（同文件无关代码省略）… */
typedef uint32_t cmpp_size_t;
/* …（同文件无关代码省略）… */
  byte ranges in a stream. It is most frequently used in API
  signatures where "if this value is negative then use
  strlen(someOtherArg) to count it".
*/
typedef int32_t cmpp_strlen_t;

/* …（同文件无关代码省略）… */
#define CMPP_SIZE_T_PFMT PRIu32
/* …（同文件无关代码省略）… */
typedef int32_t cmpp_ssize32_t;
/* …（同文件无关代码省略）… */
typedef struct cmpp cmpp;
/* …（同文件无关代码省略）… */
typedef struct cmpp_ctor_opt cmpp_ctor_opt;
/* …（同文件无关代码省略）… */
   Returns 0 on success and updates pp's error state on error.

   See: cmpp_define_v2()
   See: cmpp_undef()
*/
CMPP_EXPORT int cmpp_define_legacy(cmpp *pp, const char * zKey,
                                   char const *zVal);
/* …（同文件无关代码省略）… */

   This does _not_ affect defines made using cmpp_define_shadow().
*/
CMPP_EXPORT int cmpp_undef(cmpp *pp, const char * zKey,
                           unsigned int *nRemoved);

/**
   This works similarly to cmpp_define_v2() except that the new define
/* …（同文件无关代码省略）… */
   string fails then CMPP_RC_OOM will be returned (and pp will be
   updated appropriately).

   If pp is currently processing a script, the resulting error string
   will be prefixed with the name of the current input script and the
   line number of the directive which triggered the error.

   It is legal for zFmt to be NULL or an empty string, in which case a
   default, vague error message is used (without requiring allocation
/* …（同文件无关代码省略）… */

   See cmpp_err_get() for more information.

   FIXME: we need a different variant for WASM builds, where variadics
   aren't a usable thing.

   Potential TODO: change the error-reporting interface to support
   distinguishing from recoverable and non-recoverable errors.  "The
   problem" is that no current uses need that - they simply quit and
/* …（同文件无关代码省略）… */
CMPP_EXPORT int cmpp_errfv(cmpp *pp, int rc, char const *zFmt, va_list vars);

/**
   A variant of cmpp_errf() which is not variadic, as a consolation
   for WASM builds. zMsg may be NULL. The first nMsg bytes of the
   given string, if not NULL, are copied, or to the first NUL byte if
   zMsg is not NULL and nMsg is negative.
*/
CMPP_EXPORT int cmpp_err(cmpp *pp, int rc, char const *zMsg,
                         cmpp_strlen_t nMsg);

#if 0
/**
   Clears any error state in pp. Most cmpp APIs become no-ops if their
/* …（同文件无关代码省略）… */
CMPP_EXPORT int cmpp_slurp(cmpp_input_f xIn, void *stateIn,
                           unsigned char **pOut, cmpp_size_t * nOut);

/**
   A cleanup callback interface for use with cmpp_outputer::cleanup().
   Implementations must handle self->state appropriately for its type,
/* …（同文件无关代码省略）… */
  int errCode;
};

typedef struct cmpp_b cmpp_b;

/**
   An empty-initialized cmpp_b struct for use in const-copy
   initialization.
/* …（同文件无关代码省略）… */
*/
extern const cmpp_b cmpp_b_empty;

/**
   A class for holding error state.

/* …（同文件无关代码省略）… */
typedef struct cmpp_errinfo cmpp_errinfo;
/* …（同文件无关代码省略）… */
   or, if an allocation error happens while setting the message,
   CMPP_RC_OOM.

   To simplify certain uses, err may be NULL, in which case this simply
   returns rc.
*/
CMPP_EXPORT int cmpp_errinfo_set(cmpp_errinfo *err, int rc, char const *zMsg,
                                 cmpp_strlen_t nMsg);
/** va_list form of cmpp_errinfo_set(), supporting sqlite3_str_vappendf()'s
    formatting options. */
CMPP_EXPORT int cmpp_errinfo_setv(cmpp_errinfo *err, int rc, char const *zFmt,
/* …（同文件无关代码省略）… */
*/
CMPP_EXPORT int cmpp_b_append_str(cmpp_b * b, char const * str,
                                  cmpp_strlen_t n);

/**
   Works just like cmpp_b_append() but on allocation error it updates
/* …（同文件无关代码省略）… */
CMPP_EXPORT int cmpp_b_append4(cmpp * pp, cmpp_b * b,
                               void const * src, cmpp_size_t n);


/**
   Works like cmpp_b_append4() except that if n is negative,
   strlen() is used to calculate it.
*/
CMPP_EXPORT int cmpp_b_append4_str(cmpp * pp, cmpp_b * b,
                                   char const *str, cmpp_strlen_t n);

/**
   Appends ch to the end of os->z, expanding as necessary, and
/* …（同文件无关代码省略）… */
   Example usage:

   ```
   cmpp_b os = cmpp_b_empty;
   int rc = cmpp_stream(cmpp_input_f_FILE, stdin,
                        cmpp_output_f_b, &os);
   ...
   cmpp_b_clear(&os);
   ```
*/
CMPP_EXPORT int cmpp_output_f_b(void * buffer, void const * src,
/* …（同文件无关代码省略）… */
typedef enum cmpp_atpol_e cmpp_atpol_e;
/* …（同文件无关代码省略）… */
typedef enum cmpp_unpol_e cmpp_unpol_e;
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
typedef enum cmpp_tt_e cmpp_tt_e;
/* …（同文件无关代码省略）… */
typedef struct cmpp_token cmpp_token;
/* …（同文件无关代码省略）… */
typedef struct cmpp_tizer cmpp_tizer;
/* …（同文件无关代码省略）… */

/**
   Returns a new sqlite3_str which is prefixed (ideally) with error
   location information for tz, specifically for position z (which
   MUST be in tz's input range - that's asserted). Ownership of the
   object is transferred to the caller, who is generally expected to
   append to it as if nothing were there and eventually pass it to
   sqlite3_str_finish() to finalize it and take over its C-string
   result. Returns NULL on OOM[^1].

   If z is NULL then cmpp_tizer_errpos() is used.

   This function may assert() that z lives in tz's range - any value
   out of that range represents a serious misuse of the API.

   Very minor caveat: the returned object has no access to an sqlite3
   DB handle, which means that it is not subject to the size limits
   configured for the DB owned by any given cmpp instance. Thus it is
   possibly, but only through what may fairly be characterized as
   blatant malicious misuse, that this string can grow larger than the
   underlying sqlite3 db is configured to accept, leading to
   downstream errors if, e.g., the string is used as a the value for a
   db column.

   [^1]: sqlite3_str_new() _never_ returns NULL. On OOM it returns a
   dummy object which is in a perpetual error state. This API,
/* …（同文件无关代码省略）… */
  cmpp_arg const * next;

  /**
     If this argument came from a cmpp_itch interpreter, this will
     be set to a buffer which contains the interpolated form of the
     argument. That is, this->../z might say "$x" and this->b might
     say "3".

     In non-cmpp_itch invocations of cmpp_f() this is currently always
     0 but The Plan is to eventually offer the ability for directives
/* …（同文件无关代码省略）… */
typedef struct cmpp_dx cmpp_dx;
typedef struct cmpp__dx_pimpl cmpp__dx_pimpl;
typedef struct cmpp_d cmpp_d;
/* …（同文件无关代码省略）… */
typedef struct cmpp_f_args cmpp_f_args;
/* …（同文件无关代码省略）… */
#define cmpp_arg_equals_c(ARG,STR) \
  (sizeof("" STR)-1==(ARG)->n && 0==memcmp(STR,(ARG)->z,sizeof(STR)-1))
/* …（同文件无关代码省略）… */
#define cmpp_arg_isflag_c(ARG,STR) \
  (cmpp_arg_equals_c(ARG,STR) || cmpp_arg_equals_c(ARG, "-" STR))
/* …（同文件无关代码省略）… */

   directiveName ...args

   This function composes a new cmpp input source from that line
   (behaving as if it were prefixed with dx's current directive prefix
   if it's not already got one), processes it with
   cmpp_process_string(), redirecting the output to dest (which gets
   appended to, so be sure to cmpp_b_reuse() it if needed before
   calling this).

   To simplify common expected usage, by default the output is trimmed
   of a single newline. The flags argument, 0 or a bitmask of values
/* …（同文件无关代码省略）… */
   This is the basis of "function calls" in cmpp.

   Returns 0 on success.
*/
int cmpp_call_str(cmpp *pp,
                  unsigned char const * z,
/* …（同文件无关代码省略）… */
typedef struct cmpp_d_reg cmpp_d_reg;
/* …（同文件无关代码省略）… */
typedef struct cmpp_args cmpp_args;
/* …（同文件无关代码省略）… */
typedef struct cmpp_d_autoloader cmpp_d_autoloader;
/* …（同文件无关代码省略）… */
                                           unsigned char const **p );

/**
   Works just like cmpp_skip_space_trailing() but
   skips cmpp_skip_snl() characters.

   FIXME (2026-02-21): it does not recognize CRNL pairs as
   atomic newlines.
/* …（同文件无关代码省略）… */
typedef struct cmpp_dx_pos cmpp_dx_pos;
/* …（同文件无关代码省略）… */

/**
   Returns, via its output parameters, the current name and input line
   number of dx. This is intended for use with synthesizing names such
   as those of embedded scripts. The *zName bytes are owned by dx and
   are valid until it is destroyed.
*/
CMPP_EXPORT void cmpp_dx_src_pos_info(cmpp_dx const *dx, char const **zName,
                                      cmpp_size_t * lineNo);
/* …（同文件无关代码省略）… */
/**
   If [z,n) is unambiguously a base-10 int, sets *pOut (if not NULL)
   to that value and returns true, else returns false.
*/
CMPP_EXPORT bool cmpp_is_int(unsigned char const *z, unsigned n,
                             int *pOut);
/** 64-bit counterpart of cmpp__is_int(). */
CMPP_EXPORT bool cmpp_is_int64(unsigned char const *z, unsigned n,
                               int64_t *pOut);




/**
   The current cmpp_api_thunk::apiVersion value.  See
   cmpp_api_thunk_map.
*/
#define cmpp_api_thunk_version 20260923

/**
   This stub object is provided for cmpp interface compatibility with
/* …（同文件无关代码省略）… */
#define CMPP_OMIT_D_DB
/* …（同文件无关代码省略）… */
#define CMPP_OMIT_D_INCLUDE
/* …（同文件无关代码省略）… */
#define CMPP__DB_MAIN_NAME "cmpp"
/* …（同文件无关代码省略）… */
#define CMPP_PRIVATE static
/* …（同文件无关代码省略）… */
#  define CMPP_PLATFORM_IS_UNIX 0
#  define CMPP_PLATFORM_PLATFORM "wasm"
#  define CMPP_PATH_SEPARATOR ':'
#  define CMPP__EXPORT_NAMED(X) __attribute__((export_name(#X),used,visibility("default")))
// See also:
//__attribute__((export_name("theExportedName"), used, visibility("default")))
#  define CMPP_OMIT_FILE_IO /* potential todo but with a large footprint */
/* …（同文件无关代码省略）… */
  (i.e. error because we always build with -Wall -Werror -Wextra
  -pedantic).

  Similarly braindead, clang #defines __GNUC__.

  _Sigh_.
*/
/* …（同文件无关代码省略）… */
#define cmpp__LIST_T_empty_m {.list=0,.n=0,.na=0}
/* …（同文件无关代码省略）… */
   A single directive line from an input stream.
*/
struct CmppDLine {
  /** Line number in the source input. */
  cmpp_size_t lineNo;
  /** Start of the line within its source input, immediately
      after the directive delimiter. */
/* …（同文件无关代码省略）… */
typedef struct CmppLvl CmppLvl;
/* …（同文件无关代码省略）… */
     constructs, like #query, when they want to be able to include
     other directives in their bodies. Thus we have cmpp_dx_pos_save()
     and cmpp_dx_pos_restore() to manipulate this.
  */
  cmpp_dx_pos pos;
  /**
/* …（同文件无关代码省略）… */
  } shadow;

  struct {
    /**
       Set when we're searching for directives so that we know whether
       cmpp_out_expand() should count newlines.
     */
    unsigned short countLines;
    /**
       True if the next directive is the start of a [call].
    */
/* …（同文件无关代码省略）… */
    .ridInclPath = 0             \
  },                             \
  .flags = {                     \
    .countLines = 0,             \
    .nextIsCall = false          \
  }                              \
}
/* …（同文件无关代码省略）… */
typedef struct CmppDList_entry CmppDList_entry;
/* …（同文件无关代码省略）… */
#define CmppDList_empty_m cmpp__LIST_T_empty_m
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
    E(defDel,                                          \
      "DELETE FROM "                                   \
      CMPP__DB_MAIN_NAME ".def"                        \
      " WHERE k GLOB ?1")                              \
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
      "where r<>'' and cmpp_file_exists(fn)\n"         \
      "order by i\n"                                   \
      "limit 1;")                                      \

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
/* …（同文件无关代码省略）… */
#define cmpp__dx_delim(DX) cmpp__pp_delim(DX->pp)
#define cmpp__dx_zdelim(DX) cmpp__pp_zdelim(DX->pp)
/* …（同文件无关代码省略）… */
static inline void cmpp__err_reuse(cmpp *pp){
  cmpp_errinfo_reuse(&pp->pimpl->err);
}
/* …（同文件无关代码省略）… */
CMPP_PRIVATE int cmpp__db_init(cmpp *pp);

/**
  Returns the pp->pimpl->stmt.X corresponding to `which`, initializing it if
  needed. If it returns NULL then either this was called when pp has
  its error state set or this function will have set the error state.

  If prepEvenIfErr is true then the ppCode check is bypassed, but it
  will still fail if pp->pimpl->db is not opened or if the preparation
  itself fails.
*/
CMPP_PRIVATE sqlite3_stmt * cmpp__stmt(cmpp * pp, enum CmppStmt_e which,
                                       bool prepEvenIfErr);

/**
   A proxy for sqlite3_prepare() which supports sqlite3_str formatting
   and updates pp's error state on error.
*/
CMPP_PRIVATE int cmpp__prepare(cmpp *pp, sqlite3_stmt **pStmt,
                               const char * zSql, ...);

/**
   Reminder to self: this must return an SQLITE_... code, not a
   CMPP_RC_... code.

   On success it returns 0, SQLITE_ROW, or SQLITE_DONE. On error it
   returns another non-0 SQLITE_... code and updates pp->pimpl->err.

   This is a no-op if called when pp has an error set, returning
   SQLITE_ERROR.

   If resetIt is true, q is passed to cmpp__stmt_reset(), else the
   caller must eventually reset it.
*/
CMPP_PRIVATE int cmpp__step(cmpp * const pp, sqlite3_stmt * const q, bool resetIt);

/** Resets and clear bindings from q (if q is not NULL). */
CMPP_PRIVATE void cmpp__stmt_reset(sqlite3_stmt * const q);

/**
   Expects an SQLite result value. If it's SQLITE_OK, SQLITE_ROW, or
   SQLITE_DONE, 0 is returned without side-effects, otherwise pp->err
   is updated with pp->db's current error state. zMsgSuffix is an
   optional suffix for the error message.
*/
CMPP_PRIVATE int cmpp__db_rc(cmpp *pp, int dbRc, char const *zMsgSuffix);

/* Proxy for sqlite3_bind_int64(). */
CMPP_PRIVATE int cmpp__bind_int(cmpp *pp, sqlite3_stmt *pStmt, int col, int64_t val);

/**
   Proxy for cmpp__bind_text() which encodes val as a string.

   For queries which compare values, it's important that they all have
   the same type, so some cases where we might want an int needs to be
   bound as text instead. As of this writing (2026-08-23), arg.c
   depends on this for expression evaluation.
*/
CMPP_PRIVATE int cmpp__bind_int_text(cmpp *pp, sqlite3_stmt *pStmt, int col, int64_t val);

/* Proxy for sqlite3_bind_null(). */
CMPP_PRIVATE int cmpp__bind_null(cmpp *pp, sqlite3_stmt *pStmt, int col);

/* Proxy for sqlite3_bind_text() which updates pp->err on error. */
CMPP_PRIVATE int cmpp__bind_text(cmpp *pp,sqlite3_stmt *pStmt, int col,
                                 unsigned const char * zStr);

/* Proxy for sqlite3_bind_text() which updates pp->err on error. */
CMPP_PRIVATE int cmpp__bind_textn(cmpp *pp,sqlite3_stmt *pStmt, int col,
                                  unsigned const char *zStr, cmpp_ssize32_t len);

/**
   Adds zDir to the include path, using the given priority value (use
   0 except for the implicit cwd path which #include should (but does
/* …（同文件无关代码省略）… */
CMPP_PRIVATE char * cmpp__include_search(cmpp *pp, unsigned const char * zKey,
                                         cmpp_size_t * nVal);

 /**
   Proxy for sqlite3_str_finish() which updates pp's error state if s
   has error state. Returns s's string on success and NULL on
   error. The returned string must eventualy be passed to
   cmpp_mfree(). It also, it turns out, returns NULL if s is empty, so
   callers must check pp->err to see if NULL is an error.

   If n is not NULL then on success it is set to the byte length of
   the returned string.
*/
CMPP_PRIVATE char * cmpp_str_finish(cmpp *pp, sqlite3_str *s, int * n);

/**
   Searches pp's list of directives. If found, return it else return
   NULL. See cmpp__d_search3().
/* …（同文件无关代码省略）… */
   comparison queries will work as expected.

   Returns ppCode.
*/
CMPP_PRIVATE int cmpp__bind_arg(cmpp *pp, sqlite3_stmt * q,
                                int bindNdx, cmpp_arg const * aVal);
/* …（同文件无关代码省略）… */
static inline cmpp_size_t cmpp__strlen(char const *z, cmpp_strlen_t n){
  return n<0 ? (cmpp_size_t)strlen(z) : (cmpp_size_t)n;
}
static inline cmpp_size_t cmpp__strlenu(unsigned char const *z, cmpp_strlen_t n){
  return n<0 ? (cmpp_size_t)strlen((char const *)z) : (cmpp_size_t)n;
}
/* …（同文件无关代码省略）… */
typedef struct cmpp_argOp cmpp_argOp;
/* …（同文件无关代码省略）… */
#define cmpp__fatal(...) cmpp__fatal_base(__FILE__,__LINE__,__VA_ARGS__)
/* …（同文件无关代码省略）… */
#define g_warn(zFmt,...) g_stderr("%s:%d %s() " zFmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define g_warn0(zMsg) g_stderr("%s:%d %s() %s\n", __FILE__, __LINE__, __func__, zMsg)
/* …（同文件无关代码省略）… */
#define g_debug(PP,lvl,pfexpr) (void)0
/* …（同文件无关代码省略）… */
#define ustr_c(X) ((unsigned char const *)X)
#define ustr_nc(X) ((unsigned char *)X)
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
   Returns true if z starts with "::" not immediately followed by a
   NUL.
*/
static inline bool cmpp__has_ns_prefix(char const *z){
  return ':'==z[0] && ':'==z[1] && 0!=z[2];
}

CMPP_PRIVATE int cmpp__tt_for_group_char(int ch);
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

bool FileWrapper_chomp(FileWrapper * p){
  return cmpp_chomp(p->zContent, &p->nContent);
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
    if(0) g_warn("flush %d [%.*s]", (int)(z-zLeft), (int)(z-zLeft), zLeft); \
    cmpp__out2(pp, pOut, zLeft, (z-zLeft));                             \
  } zLeft = z
  cmpp__dx_pimpl * const dxp = pp->pimpl->dx ? pp->pimpl->dx->pimpl : NULL;
  for( ; z<zEnd && 0==ppCode; ++z ){
    zLeft = z;
    for( ;z<zEnd && 0==ppCode; ++z ){
    again:
      if( chEol==*z ){
#if 0
        broken;
        if( dxp && dxp->flags.countLines ){
          ++dxp->lineNo;
        }
#endif
        state = state_opening;
        continue;
      }
/* …（同文件无关代码省略）… */
              serr("Expecting '%s' after closing ']'.", delim->close.z);
              break;
            }
            if( nl && dxp && dxp->flags.countLines ){
              dxp->pos.lineNo += nl;
            }
            //g_warn("Found: <<%.*s>>", (int)(zb - z -1), z+1);
            cmpp_call_str(pp, z+1, (zb - z - 1),
                          cmpp_b_reuse(bCall), 0);
/* …（同文件无关代码省略）… */
CmppLvl * CmppLvl_get(cmpp_dx const *dx){
  return dx->pimpl->dxLvl.n
    ? dx->pimpl->dxLvl.list[dx->pimpl->dxLvl.n-1]
    : 0;
}
/* …（同文件无关代码省略）… */
CmppLvl * CmppLvl_push(cmpp_dx *dx){
  CmppLvl * p = 0;
  if( !dxppCode ){
    CmppLvl * const pPrev = CmppLvl_get(dx);
    p = CmppLvlList_push(dx->pp, &dx->pimpl->dxLvl);
    if( p ){
      *p = CmppLvl_empty;
      p->lineNo = dx->pimpl->dline.lineNo;
      //p->d = dx->d;
      if( pPrev ){
        p->flags = (CmppLvl_F_INHERIT_MASK & pPrev->flags);
        //if(CLvl_isSkip(pPrev)) p->flags |= CmppLvl_F_ELIDE;
      }
    }
  }
  return p;
}
/* …（同文件无关代码省略）… */
void CmppLvl_pop(cmpp_dx *dx, CmppLvl * lvl){
  CmppLvlList_pop(dx->pp, &dx->pimpl->dxLvl, lvl);
}
/* …（同文件无关代码省略）… */
void CmppLvl_elide(CmppLvl *lvl, bool on){
  if( on ) lvl->flags |=  CmppLvl_F_ELIDE;
  else     lvl->flags &= ~CmppLvl_F_ELIDE;
}
/* …（同文件无关代码省略）… */
bool CmppLvl_is_eliding(CmppLvl const *lvl){
  return lvl && !!(lvl->flags & CmppLvl_F_ELIDE);
}
/* …（同文件无关代码省略）… */
bool cmpp_dx_is_eliding(cmpp_dx const *dx){
  return CmppLvl_is_eliding(CmppLvl_get(dx));
}


char * cmpp_str_finish(cmpp *pp, sqlite3_str *s, int * n){
  char * z = 0;
  int const rc = sqlite3_str_errcode(s);
  cmpp__db_rc(pp, rc, "sqlite3_str_errcode()");
  if(0==rc){
    int const nStr = sqlite3_str_length(s);
    if(n) *n = nStr;
/* …（同文件无关代码省略）… */
  return z;
}

int cmpp__bind_int(cmpp *pp, sqlite3_stmt *pStmt, int col, int64_t val){
  return ppCode
    ? ppCode
    : cmpp__db_rc(pp, sqlite3_bind_int64(pStmt, col, val),
                     "from cmpp__bind_int()");
}

int cmpp__bind_int_text(cmpp *pp, sqlite3_stmt *pStmt, int col,
                        int64_t val){
  unsigned char buf[32];
  snprintf((char *)buf, sizeof(buf), "%" PRIi64, val);
  return cmpp__bind_textn(pp, pStmt, col, buf, -1);
}

int cmpp__bind_null(cmpp *pp, sqlite3_stmt *pStmt, int col){
  return ppCode
    ? ppCode
    : cmpp__db_rc(pp, sqlite3_bind_null(pStmt, col),
                     "from cmpp__bind_null()");
}

static int cmpp__bind_textx(cmpp *pp, sqlite3_stmt *pStmt, int col,
                            unsigned const char * zStr, cmpp_ssize32_t n,
                            void (*dtor)(void *)){
  if( 0==ppCode ){
    cmpp__db_rc(
      pp, (zStr && n)
      ? sqlite3_bind_text(pStmt, col,
                          (char const *)zStr,
                          (int)n, dtor)
      : sqlite3_bind_null(pStmt, col),
      sqlite3_sql(pStmt)
    );
  }
  return ppCode;
}

int cmpp__bind_textn(cmpp *pp, sqlite3_stmt *pStmt, int col,
                     unsigned const char * zStr, cmpp_ssize32_t n){
  return cmpp__bind_textx(pp, pStmt, col, zStr, (int)n,
                          SQLITE_TRANSIENT);
}

int cmpp__bind_text(cmpp *pp, sqlite3_stmt *pStmt, int col,
                    unsigned const char * zStr){
  return cmpp__bind_textn(pp, pStmt, col, zStr, -1);
}

#if 0
int cmpp__bind_textv(cmpp*pp, sqlite3_stmt *pStmt, int col,
                     const char * zFmt, ...){
  if( 0==p->err.code ){
    int rc;
    sqlite3_str * str = sqlite3_str_new(pp->pimpl->db.dbh);
    int n = 0;
    char * z;
    va_list va;
    va_start(va,zFmt);
    sqlite3_str_vappendf(str, zFmt, va);
    va_end(va);
    z = cmpp_str_finish(str, &n);
    cmpp__db_rc(
      pp, z
      ? sqlite3_bind_text(pStmt, col, z, n, sqlite3_free)
      : sqlite3_bind_null(pStmt, col),
      sqlite3_sql(pStmt)
    );
    cmpp_mfree(z);
  }
  return p->err.code;
}
#endif

void cmpp_outputer_set(cmpp *pp, cmpp_outputer const *out,
                       char const *zName){
  cmpp__pi(pp);
/* …（同文件无关代码省略）… */
void cmpp__outputer_swap(cmpp *pp, cmpp_outputer const *oNew,
                        cmpp_outputer *oPrev){
  if( oPrev ){
    *oPrev = pp->pimpl->out;
  }
  pp->pimpl->out = *oNew;
}
/* …（同文件无关代码省略）… */

CMPP__EXPORT(bool, cmpp_is_int)(unsigned char const *z, unsigned n,
                                int *pOut){
  if( n > 10 ) return false;
  char const * zz = (char *)z;
  char /*const sigh*/* zEnd = 0;
  int32_t d = strtol(zz, &zEnd, 10);
  if( zEnd && zEnd!=zz && *zz && n==(zEnd-zz) ){
    if( pOut ) *pOut = d;
    return true;
  }
/* …（同文件无关代码省略）… */

CMPP__EXPORT(bool, cmpp_is_int64)(unsigned char const *z, unsigned n,
                                   int64_t *pOut){
  if( n > 20 ) return false;
  char const * zz = (char *)z;
  char /*const sigh*/ * zEnd = 0;
  int64_t d = strtoll(zz, &zEnd, 10);
  if( zEnd && zEnd!=zz && *zz && n==(zEnd-zz) ){
    if( pOut ) *pOut = d;
    return true;
  }
/* …（同文件无关代码省略）… */
  }
  cmpp__FileWrapper_slurp(pp, &fw);
  q = cmpp__stmt(pp, CmppStmt_defIns, false);
  if( q && 0==cmpp__bind_textn(pp, q, 2, kvp.k.z, (int)kvp.k.n) ){
    //g_warn("zKey=%.*s", (int)kvp.k.n, kvp.k.z);
    if( pp->pimpl->flags.chompF ){
      FileWrapper_chomp(&fw);
    }
    if( fw.nContent ){
      cmpp__bind_textx(pp, q, 3, fw.zContent,
                       (cmpp_strlen_t)fw.nContent, sqlite3_free);
      fw.zContent = 0 /* transferred ownership */;
      fw.nContent = 0;
    }else{
      cmpp__bind_null(pp, q, 2);
    }
    cmpp__step(pp, q, true);
    g_debug(pp,2,("define: %s%s%s\n",
                  kvp.k.z,
                  kvp.v.z ? " with value " : "",
/* …（同文件无关代码省略）… */
int cmpp__has(cmpp *pp, const char * zName, cmpp_strlen_t nName){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defHas, false);
  if( q ){
    nName = cmpp__strlen(zName, nName);
    cmpp__bind_textn(pp, q, 1, ustr_c(zName), nName);
    if(SQLITE_ROW == cmpp__step(pp, q, true)){
      rc = 1;
    }else{
      rc = 0;
    }
    //g_debug(pp,1,("has [%s] ?= %d\n",zName, rc));
  }
  return rc;
}
/* …（同文件无关代码省略）… */
int cmpp__get_bool(cmpp *pp, unsigned const char *zName, cmpp_strlen_t nName){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defGetBool, false);
  if( q ){
    nName = cmpp__strlenu(zName, nName);
    cmpp__bind_textn(pp, q, 1, zName, nName);
    assert(0==ppCode);
    if(SQLITE_ROW == cmpp__step(pp, q, false)){
      rc = sqlite3_column_int(q, 0);
    }else{
      rc = 0;
      cmpp__affirm_undef_policy(pp, zName, nName);
    }
    cmpp__stmt_reset(q);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
int cmpp__get_int(cmpp *pp, unsigned const char * zName,
                  cmpp_strlen_t nName, int *pOut ){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defGetInt, false);
  if( q ){
    nName = cmpp__strlenu(zName, nName);
    cmpp__bind_textn(pp, q, 1, zName, nName);
    assert(0==ppCode);
    if(SQLITE_ROW == cmpp__step(pp, q, false)){
      *pOut = sqlite3_column_int(q,0);
    }else{
      cmpp__affirm_undef_policy(pp, zName, nName);
    }
    cmpp__stmt_reset(q);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
int cmpp__get_b(cmpp *pp, unsigned const char * zName,
                cmpp_strlen_t nName, cmpp_b * os, bool enforceUndefPolicy){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defGet, false);
  if( q ){
    nName = cmpp__strlenu(zName, nName);
    cmpp__bind_textn(pp, q, 1, zName, nName);
    int n = 0;
    if(SQLITE_ROW == cmpp__step(pp, q, false)){
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
    cmpp__stmt_reset(q);
    g_debug(pp,1,("get-define [%.*s] ?= %d %.*s\n",
                  nName, zName, rc, os->n, os->z));
  }
  return rc;
}
/* …（同文件无关代码省略）… */
}
#endif

#if 0
int cmpp__get(cmpp *pp, unsigned const char * zName,
              cmpp_strlen_t nName, unsigned char **zVal,
              unsigned int *nVal){
  int rc = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defGet, false);
  if( q ){
    nName = cmpp__strlenu(zName, nName);
    cmpp__bind_textn(pp, q, 1, zName, nName);
    int n = 0;
    if(SQLITE_ROW == cmpp__step(pp, q, false)){
      const unsigned char * z = sqlite3_column_text(q, 3);
      n = sqlite3_column_bytes(q, 3);
      if( nVal ) *nVal = (unsigned)n;
      *zVal = ustr_nc(sqlite3_mprintf("%.*s", n, z))
        /* TODO? Return NULL for the n==0 case? */;
      if( n && cmpp_check_oom(pp, *zVal) ){
        assert(!*zVal);
      }else{
        rc = 1;
      }
    }else{
      cmpp__affirm_undef_policy(pp, zName, nName);
      rc = 0;
    }
    cmpp__stmt_reset(q);
    g_debug(pp,1,("get-define [%.*s] ?= %d %.*s\n",
                  nName, zName, rc,
                  *zVal ? n : 0,
                  *zVal ? (char const *)*zVal : "<NULL>"));
  }
  return rc;
}
#endif

CMPP__EXPORT(int, cmpp_undef)(cmpp *pp, const char * zKey,
                                unsigned int *nRemoved){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defDel, false);
  if( q ){
    unsigned int const n = strlen(zKey);
    cmpp__bind_textn(pp, q, 1, ustr_c(zKey), (cmpp_strlen_t)n);
    cmpp__step(pp, q, true);
    if( nRemoved ){
      *nRemoved = (unsigned)sqlite3_changes(pp->pimpl->db.dbh);
    }
    g_debug(pp,2,("undefine: %.*s\n",n, zKey));
  }
  return ppCode;
}

int cmpp__include_dir_add(cmpp *pp, const char * zDir, int priority, int64_t * pRowid){
  if( pRowid ) *pRowid = 0;
  if( !ppCode && zDir && *zDir ){
    sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclPathAdd, false);
/* …（同文件无关代码省略）… */
         on top of that library (which would, e.g., replace cmpp_b
         with that one, which is more mature).
      */
      cmpp__bind_int(pp, q, 1, priority);
      cmpp__bind_textn(pp, q, 2, ustr_c(zDir), -1);
      int const rc = cmpp__step(pp, q, false);
      if( SQLITE_ROW==rc ){
        ++pp->pimpl->flags.nIncludeDir;
        if( pRowid ){
          *pRowid = sqlite3_column_int64(q, 0);
        }
      }
      cmpp__stmt_reset(q);
      /*g_warn("inclpath add: rc=%d rowid=%" PRIi64 " prio=%d %s",
        rc, pRowid ? *pRowid : 0, priority, zDir);*/
      g_debug(pp,2,("inclpath add: prio=%d %s\n", priority, zDir));
/* …（同文件无关代码省略）… */
      }
    }
    if( rc && !ppCode ){
      cmpp__db_rc(pp, rc, sqlite3_sql(q));
    }
    cmpp__stmt_reset(q);
    g_debug(pp,2,("inclpath rm #%"PRIi64 "\n", rowid));
  }
  return ppCode;
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

/**
   Scans [dx->pos.z,dx->zEnd) for a directive delimiter. Emits any
   non-delimiter output found along the way to dx->pp's output
   channel.

   This updates dx->pimpl->pos.z and dx->pimpl->pos.lineNo as it goes.

   If a delimiter is found, it sets *gotOne to true and updates
   dx->pimpl->dline to point to the remainder of that line. On no match
/* …（同文件无关代码省略）… */
  cmpp__delim const * const delim = cmpp__dx_delim(dx);
  if(!delim) {
    return cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                           "The directive delimiter stack is empty.");
  }
  bool const isCall = dxp->flags.nextIsCall
    /* If true then this call is in response to cmpp_call_str(). This
/* …（同文件无关代码省略）… */
  unsigned char const * const zEnd = dxp->zEnd;
  unsigned char const * zLeft = dxp->pos.z;
  unsigned char const * z = zLeft;
  assert(zD);
  assert(nD);
  ++dxp->flags.countLines;
  while( z<zEnd && '\n'==*z ){
    /* Skip leading newlines. We have to delay the handling of
       leading whitepace until later so that:

       |  #if
       |^^ those two spaces do not get emitted.
    */
    ++dxp->pos.lineNo;
    ++z;
  }
#define tflush                                            \
  if( z>zEnd ) z=zEnd;                                    \
  if( z>zLeft && cmpp_out_expand(dx->pp, &pi->out, zLeft, \
                                 (cmpp_size_t)(z-zLeft),  \
                                 cmpp_atpol_CURRENT) ){   \
    --dxp->flags.countLines;                              \
    return dxppCode;                                      \
  } zLeft = z

/* …（同文件无关代码省略）… */
      while( z<zEnd ){
        while((z<zEnd && '\n'==*z)
              || (z+1<zEnd && '\r'==*z && '\n'==z[1]) ){
          ++dxp->pos.lineNo;
          z += 1 + ('\r'==*z);
          atBOL = true;
        }
/* …（同文件无关代码省略）… */
         don't catch this here, we won't recognize a delimiter which
         starts on the next line. */
      z += skip;
      ++dxp->pos.lineNo;
      continue;
    }
    if( 0 ){
/* …（同文件无关代码省略）… */
    got_delim:
    /* Set up dx->pimpl->dline to encompass the whole directive line sans
       delimiter and leading spaces. */
    dline->lineNo = dxp->pos.lineNo;
    dline->zBegin = z
      /* dx->pimpl->dline starts at the directive name and extends until the
         next EOL/EOF. We don't yet know if it's a legal directive
/* …（同文件无关代码省略）… */
    }
    *gotOne = true;
    assert( !dxppCode );
    --dxp->flags.countLines;
    return 0;
  }
  /* No directives found. We're now at EOL or EOF. Flush any pending
/* …（同文件无关代码省略）… */
}

static inline void CmppDList_entry_clean(CmppDList_entry * const e){
  if( e->d.impl.dtor ){
    e->d.impl.dtor( e->d.impl.state );
  }
/* …（同文件无关代码省略）… */

void CmppDList_cleanup(CmppDList *li){
  static const CmppDList CmppDList_empty = CmppDList_empty_m;
  while( li->n ){
    CmppDList_entry_clean( li->list[--li->n] );
    cmpp_mfree( li->list[li->n] );
    li->list[li->n] = 0;
  }
  cmpp_mfree(li->list);
  *li = CmppDList_empty;
/* …（同文件无关代码省略）… */
static int CmppDList_entry_cmp_pp(const void *p1, const void *p2){
  CmppDList_entry const * eL = *(CmppDList_entry const * const *)p1;
  CmppDList_entry const * eR = *(CmppDList_entry const * const *)p2;
  uint32_t const minN = eL->d.name.n < eR->d.name.n
    ? eL->d.name.n
    : eR->d.name.n;
  int const rc = memcmp(eL->d.name.z, eR->d.name.z, minN);
  return rc ? rc : (int)eL->d.name.n - (int)eR->d.name.n;
}
/* …（同文件无关代码省略）… */
CmppDList_entry * CmppDList_search(CmppDList const * li,
                                       char const *zName){
  if( li->n > 32 ){
    /* ^^^^^^^^^ See notes itch.c:cmpp_itch_enum_search() */
    CmppDList_entry const key = {
      .d = {
        .name = {
          .z = zName,
          .n = strlen(zName)
        }
      }
    };
    CmppDList_entry const * pKey = &key;
    CmppDList_entry ** pRv
      = bsearch(&pKey, li->list, li->n, sizeof(li->list[0]),
                CmppDList_entry_cmp_pp);
    return pRv ? *pRv : 0;
  }else{
    cmpp_size_t const nName = cmpp__strlen(zName, -1);
    for( cmpp_size_t i = 0; i < li->n; ++i ){
      CmppDList_entry * const e = li->list[i];
      if( nName==e->d.name.n && 0==strcmp(zName, e->d.name.z) ){
        return e;
      }
    }
    return 0;
  }
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

cmpp_size_t cmpp_dx_guess_lineno(cmpp_dx const * dx){
  cmpp__dx_pi(dx);
  unsigned char const *zB = 0;
  unsigned char const *zE = 0;
  unsigned char const *zErr = dpi->zErrPos;
  cmpp__dx_input_range(dx, &zB, &zE);
  //g_warn("Whole script:\n%.*s", (int)(zE-zB), zB);
  assert( zB && zE && zB<=zE );
  if( !zErr ){
    if( dpi->pos.z<zE && dpi->pos.z>=zB ){
      /* This is frequently at zEnd because we read line by line and
         it can be at EOF at the end of that line. Similarly,
         [call...] contexts have a virtual EOF at the ']'. */
      zErr = dpi->pos.z;
    }else{
      zErr = dpi->dline.zBegin;
    }
  }
  return zErr
    ? 1 + cmpp_count_nl(zB, zE, zErr, NULL)
    : dpi->dline.lineNo;
}

CMPP__EXPORT(int, cmpp_dx_next)(cmpp_dx * const dx, bool * pGotOne){
  if( dxppCode ) return dxppCode;
  cmpp__dx_pimpl * const dpi = dx->pimpl;
/* …（同文件无关代码省略）… */
    return cmpp_dx_err(dx, CMPP_RC_NOT_FOUND,
                       "Unknown directive at line %"
                       CMPP_SIZE_T_PFMT ": %.*s\n",
                       cmpp_dx_guess_lineno(dx),
                       (int)bufLine->n, bufLine->z);
  }
  bool const v4 = (cmpp_f_F_ARGS_V4 & dx->d->flags);
/* …（同文件无关代码省略）… */
}

CMPP__EXPORT(int, cmpp_dx_consume)(cmpp_dx * const dx, cmpp_outputer * const os,
                    cmpp_d const * const * const dClosers,
                    unsigned nClosers,
                    cmpp_flag32_t flags){
  assert( !dxppCode );
  bool gotOne = false;
  cmpp_outputer const oldOut = dx->pp->pimpl->out;
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
  if( 0==ppCode ){
    sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_spBegin, true);
    assert( q || !"db init would have otherwise failed");
    if( q && SQLITE_DONE==cmpp__step(pp, q, true) ){
      ++pp->pimpl->flags.nSavepoint;
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
  }else{
    sqlite3_stmt * q = cmpp__stmt(pp, CmppStmt_spRollback, true);
    assert( q || !"db init would have otherwise failed");
    if( q && SQLITE_DONE==cmpp__step(pp, q, true) ){
      q = cmpp__stmt(pp, CmppStmt_spRelease, true);
      if( q && SQLITE_DONE==cmpp__step(pp, q, true) ){
          --pp->pimpl->flags.nSavepoint;
      }
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
    }else{
      sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_spRelease, true);
      assert( q || !"db init would have otherwise failed");
      if( q && SQLITE_DONE==cmpp__step(pp, q, true) ){
        --pp->pimpl->flags.nSavepoint;
      }
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
int cmpp__define_from_row(cmpp * const pp, sqlite3_stmt * const q,
                          bool defineIfNoRow, cmpp_b * bCol0){
  if( 0==ppCode ){
    int const nCol = sqlite3_column_count(q);
    assert( sqlite3_data_count(q)>0 || defineIfNoRow);
    /* Create a #define for each column */
    bool const hasRow = sqlite3_data_count(q)>0;
    if( bCol0 ) cmpp_b_reuse(bCol0);
    for( int i = 0; !ppCode && i < nCol; ++i ){
      char const * const zCol = sqlite3_column_name(q, i);
      if( hasRow ){
        unsigned char const * const zVal = sqlite3_column_text(q, i);
        int const nVal = sqlite3_column_bytes(q, i);
        cmpp_tt_e const ttype =
          cmpp__tt_for_sqlite(sqlite3_column_type(q,i));
        cmpp__define2(pp, ustr_c(zCol), -1, zVal, nVal, ttype);
        if( bCol0 ){
          cmpp_b_append4(pp, bCol0, zVal, nVal);
        }
      }else if(defineIfNoRow){
        cmpp__define2(pp, ustr_c(zCol), -1, ustr_c(""), 0, cmpp_tt_Null);
      }else{
        break;
      }
    }
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
cmpp_d const * cmpp__d_search(cmpp *pp, const char *zName){
  cmpp_d const * d = 0;//cmpp__d_search(zName);
  if( !d ){
    CmppDList_entry const * e =
      CmppDList_search(&pp->pimpl->d.list, zName);
    if( e ) d = &e->d;
  }
  return d;
}
/* …（同文件无关代码省略）… */
cmpp_d const * cmpp__d_search3(cmpp *pp, const char *zName,
                               cmpp_flag32_t what){
  cmpp_d const * d = cmpp__d_search(pp, zName);
  if( !d ){
    CmppDList_entry const * e = 0;
    if( cmpp__d_search3_F_DELAYED & what ){
      int rc = cmpp__d_delayed_load(pp, zName);
      if( 0==rc ){
        e = CmppDList_search(&pp->pimpl->d.list, zName);
      }else if( CMPP_RC_NO_DIRECTIVE!=rc ){
        assert( ppCode );
        return NULL;
      }
    }
    if( !e
        && (cmpp__d_search3_F_AUTOLOADER & what)
        && pp->pimpl->d.autoload.f
        && 0==pp->pimpl->d.autoload.f(pp, zName, pp->pimpl->d.autoload.state) ){
      e = CmppDList_search(&pp->pimpl->d.list, zName);
    }
#if CMPP_D_MODULE
    if( !e
        && !ppCode
        && (cmpp__d_search3_F_DLL & what) ){
      char * z = sqlite3_mprintf("libcmpp-d-%s", zName);
      cmpp_check_oom(pp, z);
      int rc = cmpp_module_load(pp, z, NULL);
      sqlite3_free(z);
      if( rc ){
        if( CMPP_RC_NOT_FOUND==rc ){
          cmpp__err_reuse(pp);
        }
        return NULL;
      }
      e = CmppDList_search(&pp->pimpl->d.list, zName);
    }
#endif
    if( e ) d = &e->d;
  }
  return d;
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
  cmpp_FILE * f = state;
  cmpp_size_t const rn = *n;
  *n = (cmpp_size_t)fread(dest, 1, rn, f);
  return *n==rn ? 0 : (feof(f) ? 0 : CMPP_RC_IO);
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
  unsigned char const *zB = 0;
  unsigned char const *zE = 0;
  unsigned char const *zErr = dpi->zErrPos;
  cmpp__dx_input_range(dx, &zB, &zE);
  //g_warn("Whole script:\n%.*s", (int)(zE-zB), zB);
  assert( zB );
  assert( zE );
  assert( zB<=zE );
  if( !zErr ){
    if( dpi->pos.z<zE && dpi->pos.z>=zB ){
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
  sqlite3_str_appendf(
    sstr,
    "%s%s@ %s line %" CMPP_SIZE_T_PFMT,
    dx->d ? dx->d->name.z : "",
    dx->d ? " " : "",
    (dx->sourceName
     && 0==strcmp("-", (char const *)dx->sourceName))
    ? "<stdin>"
    : (char const *)dx->sourceName,
    nl
  );
}

CMPP__EXPORT(void, cmpp_errinfo_dtor)(cmpp_errinfo *err){
/* …（同文件无关代码省略）… */
int cmpp_errinfo_setv(cmpp_errinfo *err, int rc, char const *zFmt, va_list va){
  if( err ){
    cmpp_errinfo_reuse(err);
    err->code = 0 /* some APIs become no-ops if pp->pimpl->err.code
                     is set, so delay setting this. */;
    if( 0==rc ) return rc;
    if( CMPP_RC_OOM==rc ){
    oom:
      err->zStatic ="An allocation failed.";
      return err->code = CMPP_RC_OOM;
    }
    assert( !err->zStatic );
    if( zFmt && *zFmt ){
      sqlite3_str * sstr = sqlite3_str_new(0);
      sqlite3_str_vappendf(sstr, zFmt, va);
      int const nz = sqlite3_str_length(sstr);
      char * const z = sqlite3_str_finish(sstr);
      sstr = 0;
      if( !z ){
        goto oom;
      }
      if( err->b.z ){
        cmpp_b_append_str(&err->b, z, nz);
        sqlite3_free(z);
        if( err->b.errCode ) goto oom;
      }else{
        err->b.z = (unsigned char*)z;
        err->b.n = (cmpp_size_t)nz;
        err->b.nAlloc = err->b.n + 1/*NUL byte*/;
      }
      err->zStatic = (char const *)err->b.z;
    }else{
      err->zStatic = "No error info provided.";
    }
    err->code = rc;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
  return rc;
}

int cmpp_errfv(cmpp *pp, int rc, char const *zFmt, va_list va){
  if( pp ){
    cmpp_errinfo * const err = &pp->pimpl->err;
    cmpp_errinfo_reuse(err);
    err->code = 0 /* some APIs become no-ops if pp->pimpl->err.code
                     is set, so delay setting this. */;
    if( 0==rc ) return rc;
    if( CMPP_RC_OOM==rc ){
    oom:
/* …（同文件无关代码省略）… */
    assert( !err->zStatic );
    if( pp->pimpl->dx || (zFmt && *zFmt) ){
      sqlite3_str * const sstr = cmpp__sqlite3_str_new(pp);
      if( pp->pimpl->dx ){
        cmpp__dx_append_script_info(pp->pimpl->dx, sstr);
        sqlite3_str_append(sstr, ": ", 2);
/* …（同文件无关代码省略）… */
      }else{
        sqlite3_str_appendf(sstr, "No error info provided.");
      }
      int const nz = sqlite3_str_length(sstr);
      char * z = sqlite3_str_finish(sstr);
      if( !z ){
        goto oom;
      }
#if 0
      cmpp_b_append(&err->b, z, nz);
      sqlite3_free(z);
/* …（同文件无关代码省略）… */
          goto oom;
        }
        sqlite3_free(z);
        z = 0;
      }else{
        err->b.z = (unsigned char*)z;
        err->b.n = (cmpp_size_t)nz;
        err->b.nAlloc = err->b.n + 1 /*NUL terminator*/;
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
  pp->pimpl->d.autoload = cmpp_d_autoloader_empty;
}

//CMPP_WASM_EXPORT no - variadic
int cmpp_dx_errf(cmpp_dx *dx, int rc,
                    char const *zFmt, ...){
/* …（同文件无关代码省略）… */
  return cmpp_errfv(dx->pp, rc, zFmt, vargs);
}

CMPP__EXPORT(int, cmpp_err)(cmpp *pp, int rc, char const *zMsg,
                                 cmpp_strlen_t nMsg){
  if( zMsg && *zMsg ){
    if( nMsg<0 ) nMsg = zMsg ? strlen(zMsg) : 0;
    return cmpp_errf(pp, rc, "%.*s", (int)nMsg, zMsg);
  }
  return cmpp_errf(pp, rc, 0);

}

//no: CMPP_WASM_EXPORT
char * cmpp_path_search(cmpp *pp,
                        char const *zPath,
                        char pathSep,
                        char const *zBaseName,
                        char const *zExt){
  char * zrc = 0;
  if( !ppCode ){
    sqlite3_stmt * const q =
      cmpp__stmt(pp, CmppStmt_selPathSearch, false);
    if( q ){
      unsigned char sep[2] = {pathSep, 0};
      cmpp__bind_text(pp, q, 1, ustr_c(zBaseName));
      cmpp__bind_text(pp, q, 2, sep);
      cmpp__bind_text(pp, q, 3, ustr_c((zExt ? zExt : "")));
      cmpp__bind_text(pp, q, 4, ustr_c((zPath ? zPath: "")));
      int const dbrc = cmpp__step(pp, q, false);
      if( SQLITE_ROW==dbrc ){
        unsigned char const * s = sqlite3_column_text(q, 1);
        zrc = sqlite3_mprintf("%s", s);
        cmpp_check_oom(pp, zrc);
      }
      cmpp__stmt_reset(q);
    }
  }
  return zrc;
}
/* …（同文件无关代码省略）… */
static bool get_flag_val(int argc,
                        char const * const * argv, int * ndx,
                        char const **zVal){
  char const * zEq = strchr(argv[*ndx], '=');
  if( zEq ){
    *zVal = zEq+1;
    return 1;
  }else if(*ndx+1>=argc){
    return 0;
  }else{
    *zVal = argv[++*ndx];
    return 1;
  }
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
  return cmpp__strlenu(z, n);
}


CMPP__EXPORT(void,cmpp_dx_src_pos_info)(
  cmpp_dx const *dx, char const **zName,
  cmpp_size_t * lineNo
/* …（同文件无关代码省略）… */
  }
}

void cmpp__dump_defines(cmpp *pp, cmpp_FILE * fp, int bIndent){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_defSelAll, false);
  if( q ){
/* …（同文件无关代码省略）… */
      fprintf(fp, "%s%.*s = [%s] %.*s\n", bIndent ? "\t" : "",
              nK, zK, zTt, nV, zV);
    }
    cmpp__stmt_reset(q);
  }
}

/**
   This is what was originally the main() of cmpp v1, back when it was
   a monolithic app. It still serves as the driver for main() but is
/* …（同文件无关代码省略）… */
  int nFile = 0    /* number of files/-e scripts seen */;

#define ARGVAL if( !zVal && !get_flag_val(argc, argv, &i, &zVal) ){ \
    cmpp__err(pp, CMPP_RC_MISUSE, "Missing value for flag '%s'", \
                  argv[i]);                                          \
    break;                                                           \
  }
#define M(X) cmpp__arg_is_flag(X, zArg, &zVal)
#define ISFLAG(X) else if(M(X))
/* …（同文件无关代码省略）… */
#define DOIT if(1==doIt)
/* …（同文件无关代码省略）… */
        if(!*zArg){
          cmpp__err(pp,CMPP_RC_MISUSE,"Missing key for -U");
        }else DOIT {
            cmpp_undef(pp, zArg, NULL);
        }
      }else if('I'==*zArg){
        ++zArg;
/* …（同文件无关代码省略）… */
#define PC 0x80 /* pad character */
#define WS 0x81 /* whitespace */
#define ND 0x82 /* Not above or digit-value */
/* …（同文件无关代码省略）… */
#define BX_NUMERAL(dv) (b64Numerals[(uint8_t)(dv)])
/* …（同文件无关代码省略）… */
#define B64_DARK_MAX 72
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
    : (b->n<nn ? -1 : 1/* prefix match: shortest first */);
}

#undef PC
#undef WS
#undef ND
/* …（同文件无关代码省略）… */
#undef BX_NUMERAL
#undef B64_DARK_MAX

/**
   A proxy for sqlite3_prepare() which updates pp->pimpl->err on error.
*/
int cmpp__prepare(cmpp *pp, sqlite3_stmt **pStmt,
                  const char * zSql, ...){
  /* We need for pp->pimpl->stmt.sp* to work regardless of pending
     errors so that we can, when appropriate, create the rollback
     statements. Thus we don't check ppCode before starting. */
  sqlite3_str * str = sqlite3_str_new(pp->pimpl->db.dbh);
  char * z = 0;
  int n = 0;
  va_list va;
  assert( pp->pimpl->db.dbh );
  va_start(va, zSql);
  sqlite3_str_vappendf(str, zSql, va);
  va_end(va);
  z = cmpp_str_finish(pp, str, &n);
  if( z ){
    int const rc = sqlite3_prepare_v2(pp->pimpl->db.dbh, z, n, pStmt, 0);
    /* TODO (2026-08-29): add the SQL to the message if it's not
       SQLITE_STEP or SQLITE_DONE. */
    if( cmpp__db_rc(pp, rc, z) ){
      g_warn("SQLite rc=%d Offending SQL: %s",rc, z);
    }
    sqlite3_free(z);
  }
  return ppCode;
}

sqlite3_stmt * cmpp__stmt(cmpp * pp, enum CmppStmt_e which,
                          bool prepEvenIfErr){
  if( !pp->pimpl->db.dbh && cmpp__db_init(pp) ) return NULL;
/* …（同文件无关代码省略）… */
  assert( q );
  assert( zSql && *zSql );
  if( !*q && (!ppCode || prepEvenIfErr) ){
    cmpp__prepare(pp, q, "%s", zSql);
  }
  return *q;
}

void cmpp__stmt_reset(sqlite3_stmt * const q){
  if( q ){
    sqlite3_clear_bindings(q);
    sqlite3_reset(q);
  }
}

static inline int cmpp__stmt_is_sp(cmpp const * const pp,
                                   sqlite3_stmt const * const q){
  return q==pp->pimpl->stmt.spBegin
    || q==pp->pimpl->stmt.spRelease
    || q==pp->pimpl->stmt.spRollback;
}

int cmpp__step(cmpp * const pp, sqlite3_stmt * const q, bool resetIt){
  int rc = SQLITE_ERROR;
  assert( q );
  if( !ppCode || cmpp__stmt_is_sp(pp,q) ){
    rc = sqlite3_step(q);
    cmpp__db_rc(pp, rc, sqlite3_sql(q));
  }
  if( resetIt /* even if ppCode!=0 */ ) cmpp__stmt_reset(q);
  assert( 0!=rc );
  return rc;
}

/**
   Expects an SQLITE_... result code and returns an approximate match
   from cmpp_rc_e. It specifically treats SQLITE_ROW and SQLITE_DONE
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

int cmpp__db_rc(cmpp *pp, int dbRc, char const *zMsg){
  switch(dbRc){
    case 0:
    case SQLITE_DONE:
/* …（同文件无关代码省略）… */
    case SQLITE_NOMEM:
      return CMPP_RC_OOM;
    default:
      return cmpp_errf(
        pp, cmpp__db_errcode(pp->pimpl->db.dbh, dbRc),
        "SQLite error #%d: %s%s%s",
        dbRc,
        pp->pimpl->db.dbh
        ? sqlite3_errmsg(pp->pimpl->db.dbh)
        : "<no db handle>",
        zMsg ? ": " : "",
        zMsg ? zMsg : ""
      );
  }
}

/* …（同文件无关代码省略）… */
    assert( q );
    nKey = cmpp__strlenu(zKey, nKey);
    nVal = cmpp__strlenu(zVal, nVal);
    if( 0==cmpp__bind_textn(pp, q, 2, zKey, (int)nKey)
        && 0==cmpp__bind_int(pp, q, 1, tType) ){
      //g_stderr("zKey=%s\nzVal=%s\nzEq=%s\n", zKey, zVal, zEq);
      /* TODO? if tType==cmpp_TT_Blob, bind it as a blob */
      if( zVal ){
        if( nVal ){
          cmpp__bind_textn(pp, q, 3, zVal, (int)nVal);
        }else{
          /* Arguable */
          cmpp__bind_null(pp, q, 3);
        }
      }else{
        cmpp__bind_int(pp, q, 3, 1);
      }
      cmpp__step(pp, q, resetStmt);
      g_debug(pp,2,("define: %s [%s]=[%.*s]\n",
                    cmpp_tt_cstr(tType), zKey, (int)nVal, zVal));
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
    default:
      break;
  }
  if( 0==cmpp__bind_textn(pp, q, 2, kvp.k.z, kvp.k.n)
      && 0==cmpp__bind_int(pp, q, 1, ttype) ){
    //g_stderr("zKey=%s\nzVal=%s\nzEq=%s\n", zKey, zVal, zEq);
    switch( ttype ){
      case cmpp_tt_IntDec:
        cmpp__bind_int(pp, q, 3, intCheck);
        break;
      case cmpp_tt_Null:
        cmpp__bind_null(pp, q, 3);
        break;
      default:
        cmpp__bind_textn(pp, q, 3, kvp.v.z, (int)kvp.v.n);
        break;
    }
    cmpp__step(pp, q, true);
    g_debug(pp,2,("define: [%.*s]=[%.*s]\n",
                  kvp.k.n, kvp.k.z,
                  kvp.v.n, kvp.v.z));
/* …（同文件无关代码省略）… */
      *pId = sqlite3_column_int64(q, 0);
      assert( *pId );
    }
    cmpp__stmt_reset(q);
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
                          cmpp_strlen_t nKey, int64_t id){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_sdefDel, false);
  if( q ){
    cmpp__bind_textn(pp, q, 1, zKey, (int)nKey);
    cmpp__bind_int(pp, q, 2, id);
    cmpp__step(pp, q, true);
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
    cmpp__db_rc(pp, rc, pi->db.zName
                ? pi->db.zName
                : ":memory:");
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
  if( cmpp__db_rc(pp, rc, "Installing tracer failed") ){
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
  if( cmpp__db_rc(pp, rc, "UDF registration failed.") ){
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
    cmpp__bind_int(pp, q, 1, cmpp_tt_ ## N);                            \
    cmpp__bind_textn(pp, q, 2,                                          \
                     ustr_c("cmpp_tt_ " # N), sizeof("cmpp_tt_" # N)-1); \
    if( STR ) cmpp__bind_textn(pp, q, 3, ustr_c(STR), sizeof(STR)-1);   \
    else cmpp__bind_null(pp, q, 3);                                     \
    if( SQLITE_DONE!=cmpp__step(pp, q, true) ) return ppCode;
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
static int cmpp__dx_err_just_once(cmpp_dx *dx, cmpp_arg const *arg){
  return cmpp_dx_errf(dx, CMPP_RC_MISUSE, "'%s' may only be used once.",
                         arg->z);
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
static void cmpp_f_error(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  const char *zBegin = (char const *)dx->args.z;
  unsigned n = (unsigned)dx->args.nz;
  if( n>2 && (('"' ==*zBegin || '\''==*zBegin) && zBegin[n-1]==*zBegin) ){
    ++zBegin;
    n -= 2;
  }
  if( n ){
    cmpp_dx_errf(dx, CMPP_RC_ERROR, "%.*s", n, zBegin);
  }else{
    cmpp_dx_errf(dx, CMPP_RC_ERROR, "(no additional info)");
  }
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
        int d = 0;
        if( 0==cmpp__arg_evalSubToInt(args->pp, arg, &d) ){
          char exprBuf[32] = {0};
          cmpp_size_t nVal =
            (cmpp_size_t)snprintf(&exprBuf[0],
                                  sizeof(exprBuf), "%d", d);
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
  for( unsigned i = 1; i < args->argc && 0==dxppCode; ++i ){
    cmpp_arg const * arg = args->argv[i];
    if( 0 ){
      g_stderr("  %s: %s %p n=%d %.*s\n", args->d->name.z,
               cmpp__tt_cstr2(arg->ttype, true), arg->z,
               (int)arg->n, (int)arg->n, arg->z);
    }
    if( cmpp_tt_Word==arg->ttype ){
#if 0
      /* Too strict? */
      if( 0==cmpp__legal_key_check(dx->pp, arg->z,
                                   (cmpp_strlen_t)arg->n, false) ) {
        cmpp_undef(dx->pp, (char const *)arg->z);
      }
#else
      cmpp_undef(dx->pp, (char const *)arg->z, NULL);
#endif
    }else{
      cmpp_errf(dx->pp, CMPP_RC_MISUSE, "Invalid arg for %s: %s",
                args->d->name.z, arg->z);
    }
  }
}
/* …（同文件无关代码省略）… */
static void cmpp_f_eval(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  if( 2!=args->argc  ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                    "#eval requires one argument");
    return;
  }
  cmpp_arg const * const arg = args->argv[1];
  cmpp_b * const b = arg->n ? cmpp_b_borrow(dx->pp) : 0;
  if( b ){
    /* We need to collect the line info before processing the arg so
       that it points to the start of the script. */
    sqlite3_str * const sstr = cmpp__sqlite3_str_new(args->pp);
    if( sstr ){
      cmpp__dx_append_script_info(dx, sstr);
      char * const z = sqlite3_str_finish(sstr);
      if(0==cmpp_check_oom(args->pp, z)
         && 0==cmpp_arg_interpolate(args->pp, arg, b,
                             cmpp_arg_interpolate_BRACE_CALL)
         && b->n){
        cmpp_process_string(args->pp, z ? z : args->d->name.z,
                            b->z, b->n);
      }
      sqlite3_free(z);
    }
    cmpp_b_return(args->pp, b);
  }
  return;
}
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
  if( dx->args.arg0 ){
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
  //g_debug(args->pp,1,("#once key: %s", b->z));
  int const had = cmpp__has(args->pp, (char const *)b->z, b->n);
  if( dxppCode ) goto end;
  else if( had ){
    /* Consume until #/once in elide mode. */
    CmppLvl * const lvl = CmppLvl_push(dx);
    if( lvl ){
      CmppLvl_elide(lvl, true);
      cmpp_outputer devNull = cmpp_outputer_empty;
      cmpp_dx_consume(dx, &devNull, &d->closer, 1,
                      cmpp_dx_consume_F_PROCESS_OTHER_D);
      CmppLvl_pop(dx, lvl);
    }
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
  if( q && 0==cmpp__bind_text(pp, q, 1, zName) ){
    if(SQLITE_ROW == cmpp__step(pp, q, true)){
      rc = 1;
    }else{
      rc = 0;
/* …（同文件无关代码省略）… */
char * cmpp__include_search(cmpp *pp, unsigned const char * zKey,
                            cmpp_size_t * nVal){
  char * zName = 0;
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclSearch, false);
  if( nVal ) *nVal = 0;
  if( q && 0==cmpp__bind_text(pp, q, 1, zKey) ){
    int const rc = cmpp__step(pp, q, false);
    if(SQLITE_ROW==rc){
      const unsigned char * z = sqlite3_column_text(q, 0);
      int const n = sqlite3_column_bytes(q,0);
      zName = n ? sqlite3_mprintf("%.*s", n, z) : 0;
      if( n ) cmpp_check_oom(pp, zName);
      if( nVal ) *nVal = n;
    }
    cmpp__stmt_reset(q);
  }
  return zName;
}
/* …（同文件无关代码省略）… */
static int cmpp__include_rm(cmpp *pp, unsigned const char * zKey){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclDel, false);
  if( q ){
    cmpp__bind_text(pp, q, 1, ustr_c(zKey));
    cmpp__step(pp, q, true);
    g_debug(pp,2,("incl rm [%s]\n", zKey));
  }
  return ppCode;
/* …（同文件无关代码省略）… */
static int cmpp__including_add(cmpp *pp, unsigned const char * zKey,
                               unsigned const char * zSrc, cmpp_size_t srcLine){
  sqlite3_stmt * const q = cmpp__stmt(pp, CmppStmt_inclIns, false);
  if( q ){
    cmpp__bind_text(pp, q, 1, zKey);
    cmpp__bind_text(pp, q, 2, zSrc);
    cmpp__bind_int(pp, q, 3, srcLine);
    cmpp__step(pp, q, true);
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
typedef struct CmppIfState CmppIfState;
/* …（同文件无关代码省略）… */
static void cmpp_f_if(cmpp_f_args const * xargs){
  cmpp_dx * const dx = xargs->dx;
  /* Reminder to self:

     We need to be able to recurse, even in skip mode, for #if nesting
     to work.  That's not great because it means we are evaluating
     stuff we ideally should be skipping over, but it's keeping the
     current tests working as-is. We can/do, however, avoid evaluating
     expressions and such when recursing via skip mode. If we can
     eliminate that here, by keeping track of the #if stack depth,
     then we can possibly eliminate the whole CmppLvl_F_ELIDE
     flag stuff.

     The more convoluted version 1 #if (which this replaced not hours
     ago) kept track of the skip state across a separate directive
     function for #if and #/if. That was more complex but did avoid
     having to recurse into #if in order to straighten out #elif and
     #else. Update: tried a non-recursive variant involving moving
     this function's gotTruth into the CmppLvl object() and
     managing the CmppLvl stack here, but it just didn't want to
     work for me and i was too tired to figure out why.
  */
  int gotTruth = 0 /*expr result*/;
  CmppIfState const * const cis = xargs->d->impl.state;
  cmpp_d const * dClosers[] = {
    cis->dElif, cis->dElse, cis->dEndif
  };
  CmppLvl * lvl = 0;
  CmppDLine const dline = dx->pimpl->dline;
  cmpp_args args = cmpp_args_empty;
  char delim[20] = {0};
#define skipOn CmppLvl_elide((lvl), true)
#define skipOff CmppLvl_elide((lvl), false)

  assert( xargs->d==cis->dIf );
  if( !dx->args.arg0 ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE, "Expecting an expression.");
    return;
  }
  snprintf(delim, sizeof(delim), "%s", cmpp_dx_delim(dx));
  delim[sizeof(delim)-1] = 0;
  lvl = CmppLvl_push(dx);
  if( !lvl ) goto end;
  if( cmpp_dx_is_eliding(dx) ){
    gotTruth = 1;
  }else if( cmpp__args_evalToInt(xargs->pp, &dx->pimpl->args,
                                 &gotTruth) ){
    goto end;
  }else if( !gotTruth ){
    skipOn;
  }

  cmpp_d const * dPrev = xargs->d;
  cmpp_outputer devNull = cmpp_outputer_empty;
  while( !dxppCode ){
    dPrev = dx->d;
    bool const isFinal = dPrev==cis->dElse
      /* true if expecting an #/if. */;
    if( cmpp_dx_consume(dx,
                        CmppLvl_is_eliding(lvl) ? &devNull : NULL,
                        isFinal ? &cis->dEndif : dClosers,
                        isFinal ? 1 : sizeof(dClosers)/sizeof(dClosers[0]),
                        cmpp_dx_consume_F_PROCESS_OTHER_D) ){
      break;
    }
    cmpp_d const * const d2 = dx->d;
    if( !d2 ){
      dxserr("Reached end of input in an untermined %s%s opened "
             "at line %" CMPP_SIZE_T_PFMT ".",
             delim, cis->dIf->name.z, dline.lineNo);
    }
    if( d2==cis->dEndif ){
      break;
    }else if( isFinal ){
      assert(!"cannot happen - caught by consume()");
      dxserr("Expecting %s%s to close %s%s.",
             delim, cis->dEndif->name.z,
             delim, dPrev->name.z);
      break;
    }else if( gotTruth ){
      skipOn;
      continue;
    }else if( d2==cis->dElif ){
      if( 0==cmpp_dx_args_parse(dx, &args)
          && 0==cmpp__args_evalToInt(xargs->pp, &args, &gotTruth) ){
        if( gotTruth ) skipOff;
        else skipOn;
      }
      continue;
    }else{
      assert( d2==cis->dElse
              && "Else (haha!) we cannot have gotten here" );
      skipOff;
      continue;
    }
    assert(!"unreachable");
  }

#undef skipOff
#undef skipOn
end:
  cmpp_args_cleanup(&args);
  if( lvl ){
    bool const lvlIsOk = CmppLvl_get(dx)==lvl;
    CmppLvl_pop(dx, lvl);
    if( !lvlIsOk && !dxppCode ){
      assert(!"i naively believe that this is not possible");
      cmpp_dx_errf(dx, CMPP_RC_SYNTAX,
                      "Mis-terminated %s%s opened at line "
                      "%" CMPP_SIZE_T_PFMT ".",
                      delim, cis->dIf->name.z, dline.lineNo);
    }
  }
  return;
}
/* …（同文件无关代码省略）… */
static void cmpp_f_if_dangler(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  CmppIfState const * const cis = args->d->impl.state;
  char const *zDelim = cmpp_dx_delim(dx);
  cmpp_dx_errf(dx, CMPP_RC_SYNTAX,
              "%s%s with no matching %s%s",
              zDelim, args->d->name.z,
              zDelim, cis->dIf->name.z);
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
  SO(CmppLvl);
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
  cmpp_arg const * arg = dx->args.arg0;
  if( !arg ){
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
  for( ; arg; arg = arg ? arg->next : NULL ){
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
                        "'policy' or delimiter'.");
      }
      goto end;
    }/* checkedCallForm */
    if( !argDelimC && op_none==op ){
      /* Look for push|pop. */
      if( cmpp_arg_equals_c(arg, "pop") ){
        arg = arg->next;
        if( !arg ){
          cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                          "'pop' expects arguments of 'policy' "
                          "and/or 'delimiter' and/or 'both'.");
          goto end;
        }
        for( ; arg; arg = arg->next ){
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
                            "Invalid argument to 'pop': ", arg->z);
            goto end;
          }
        }
        assert( !arg );
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
      arg = arg->next;
      if( !arg ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "'policy' requires a policy name argument.");
        goto end;
      }
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
      argDelimO = arg->next;
      argDelimC = argDelimO ? argDelimO->next : NULL;
      if( !argDelimC ){
        cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                        "'delimiter' requires two arguments.");
        goto end;
      }
      arg = argDelimC->next;
      continue;
    }
    if( op_pop!=op ){
      if( cmpp_arg_equals_c(arg,"<<") ){
        if( arg->next ) {
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
                      cmpp_dx_consume_F_PROCESS_OTHER_D)
        /* !Invalidates argDelimO and argDelimC! */;
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
  cmpp_dx * const dx = args->dx;
  int rv = 0;
  assert( dx->args.z );
  if( 0 ){
    g_stderr("%s() argc=%d arg0 [%.*s]\n", __func__, dx->args.argc,
             dx->args.arg0->n, dx->args.arg0->z);
    g_stderr("%s() dx->args.z [%.*s]\n", __func__,
             (int)dx->args.nz, dx->args.z);
  }
  if( !dx->args.argc ){
    dxserr("An empty expression is not permitted.");
    return;
  }
#if 0
  for( cmpp_arg const * a = dx->args.arg0; a; a = a->next ){
    g_stderr("got type=%s n=%u z=%.*s\n",
             cmpp__tt_cstr2(a->ttype, true),
             (unsigned)a->n, (int)a->n, a->z);
  }
#endif
  if( 0==cmpp__args_evalToInt(args->pp, &dx->pimpl->args, &rv) ){
    if( 'a'==args->d->name.z[0] ){
      if( !rv ){
        cmpp_errf(args->pp, CMPP_RC_ASSERT,
                     "Assertion failed: %s",
                     args->raw.z);
      }
    }else{
      char buf[60];
      snprintf(buf, sizeof(buf), "%d\n", rv);
      cmpp_out_raw(args->pp, buf, strlen(buf));
    }
  }
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
static void cmpp_f_attach(cmpp_f_args const * args){
  if( 4!=args->argc ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE,
              "%s expects: STRING as NAME",
              args->d->name.z);
    return;
  }
  cmpp_b * bDbFile = 0;
  cmpp_b * bSchema = 0;
  for( unsigned i = 1; i < args->argc; ++i ){
    cmpp_arg const * arg = args->argv[i];
    if( !bDbFile ){
      bDbFile = cmpp_b_borrow(args->pp);
      if( !bDbFile ) goto end;
      if( 0==cmpp_arg_interpolate(args->pp, arg, bDbFile,
                           cmpp_arg_interpolate_BRACE_CALL)
          && !bDbFile->n ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Empty db file name is not permitted. "
                  "If '%s' is intended as a value, "
                  "it should be quoted.", arg->z);
        goto end;
      }
      if( ++i == args->argc
          || !cmpp_arg_equals_c(args->argv[i], "as") ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Expecting 'as' after db file name.");
        goto end;
      }
      continue;
    }
    if( !bSchema ){
      bSchema = cmpp_b_borrow(args->pp);
      if( !bSchema ) goto end;
      if( 0==cmpp_arg_interpolate(args->pp, arg, bSchema,
                           cmpp_arg_interpolate_BRACE_CALL)
          && !bSchema->n ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Empty db schema name is not permitted."
                  "If '%s' is intended as a value, "
                  "it should be quoted.",
                  arg->z);
        goto end;
      }
      continue;
    }
    cmpp_errf(args->pp, CMPP_RC_MISUSE,
              "Unhandled argument: %s", arg->z);
    goto end;
  }
  if( !bSchema ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE, "Missing schema name.");
    goto end;
  }
  sqlite3_stmt * const q =
    cmpp__stmt(args->pp, CmppStmt_dbAttach, false);
  if( q ){
    cmpp__bind_textn(args->pp, q, 1, bDbFile->z, bDbFile->n);
    cmpp__bind_textn(args->pp, q, 2, bSchema->z, bSchema->n);
    cmpp__step(args->pp, q, true);
  }
end:
  cmpp_b_return(args->pp, bDbFile);
  cmpp_b_return(args->pp, bSchema);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_detach(cmpp_f_args const * args){
  if( 2!=args->argc ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE,
              "%s expects: NAME", args->d->name.z);
    return;
  }
  cmpp_b * const b = cmpp_b_borrow(args->pp);
  if( !b ) return;
  if( cmpp_arg_interpolate(args->pp, args->argv[1], b,
                    cmpp_arg_interpolate_BRACE_CALL) ){
    goto end;
  }
  if( b->n ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE,
              "Empty db schema name is not permitted.");
    goto end;
  }
  sqlite3_stmt * const q =
    cmpp__stmt(args->pp, CmppStmt_dbDetach, false);
  if( q ){
    cmpp__bind_textn(args->pp, q, 1, b->z, b->n);
    cmpp__step(args->pp, q, true);
  }
end:
  cmpp_b_return(args->pp, b);
}
/* …（同文件无关代码省略）… */
static void cmpp_f_delimiter(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  cmpp_arg const * arg = dx->args.arg0;
  enum ops { op_none, op_set, op_push, op_pop };
  enum ops op = op_none;
  cmpp_arg const * argD = 0;
  bool doHeredoc = false;
  for( ; arg; arg = arg->next ){
    if( op_none==op ){
      /* Look for push|pop. */
      if( cmpp_arg_equals_c(arg, "push") ){
        op = op_push;
        continue;
      }else if( cmpp_arg_equals_c(arg, "pop") ){
        if( arg->next ){
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
      }else if( arg->next ){
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
      cmpp_delimiter_push(args->pp, (char const*)argD->z);
    }else{
      assert( op_set==op );
      if( doHeredoc ) cmpp_delimiter_push(args->pp, z);
      else cmpp_delimiter_set(args->pp, z);
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
static void cmpp_f_experiment(cmpp_f_args const * args){
  cmpp_dx * const dx = args->dx;
  void * st = args->d->impl.state;
  (void)st;
  g_warn("raw args: %s", args->raw.z);
  g_warn("argc=%u", args->argc);
  g_warn("isCall=%d\n", args->isDxCall);

  if( 1 ){
    g_warn("argc=%u", args->argc);
    for(unsigned i = 0; i < args->argc; ++i ){
      cmpp_arg const * const a = args->argv[i];
      g_stderr("got type=%s, n=%u z=%.*s\n",
               cmpp__tt_cstr2(a->ttype, true),
               (unsigned)a->n, (int)a->n, a->z);
    }
  }

  if( 0 ){
    cmpp__dump_sizeofs();
  }

  if( 0 ){
    cmpp_tizer tz = cmpp_tizer_empty;
    cmpp_token const * const tok = &tz.token;
    int rc = 0;
    cmpp_b * const b = cmpp_b_borrow(args->pp);
    assert( b );
    cmpp_b_append4(args->pp, b, args->raw.z, args->raw.n);
    cmpp_tizer_init(&tz, &args->pp->pimpl->err, b->z, b->n);
    g_warn0("Begin tizer test");
    cmpp_b * const bc = cmpp_b_borrow(args->pp);
    const cmpp_flag32_t cflags = 0;//cmpp_token_content_WHOLE;
    while( 0==(rc = cmpp_tizer_next(&tz, 0))
           && cmpp_tt_EOF!=tok->ttype ){
      cmpp_token_content(&tz, 0, bc, cflags);
      g_warn("token type=%d: %.*s ==> %.*s",
             tok->ttype, (int)tok->n, tok->z, (int)bc->n, bc->z);
    }
    cmpp_b_return(args->pp, bc);
    g_warn("End tizer test. rc=%d", dxppCode);
    cmpp_dx_errf(dx, 0, 0);
    cmpp_tizer_dtor(&tz);
    cmpp_b_return(args->pp, b);
    if(rc) return;
  }
}
/* …（同文件无关代码省略）… */
static
int cmpp__consume_sql_args(cmpp *pp, cmpp_f_args const *args,
                           unsigned int * i,
                           cmpp_arg const **pBind){
  if( 0==ppCode ){
#define nextArg (++*i<args->argc ? args->argv[*i] : 0)
    *pBind = 0;
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
      *pBind = 0;
    }
#undef nextArg
  }
  return ppCode;
}
/* …（同文件无关代码省略）… */
    int const bindNdx =
      sqlite3_bind_parameter_index(q, (char const*)zKey);
    if( bindNdx ){
      cmpp__bind_textn(pp, q, bindNdx, zVal, nVal);
    }else{
      cmpp_errf(pp, CMPP_RC_RANGE, "Invalid bind name: %.*s",
                   (int)nKey, zKey);
/* …（同文件无关代码省略）… */
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
static void cmpp_f_query(cmpp_f_args const * args){
  if( !args->argc ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE,
              "Expecting one or more arguments");
    return;
  }
  cmpp * const pp = args->pp;
  cmpp_dx * const dx = args->dx;
  sqlite3_stmt * q = 0;
  cmpp_b * const obBody = cmpp_b_borrow(args->pp);
  cmpp_b * const sql = cmpp_b_borrow(args->pp);
  cmpp_outputer obNull = cmpp_outputer_empty;
  int nChomp = 0;
  int nUnchomp = 0;
  bool spStarted = false;
  bool seenDefine = false;
  bool seenEmit = false;
  bool batchMode = false;
  cmpp_arg const * aBind = 0;
  cmpp_d const * const dNoRows = args->d->impl.state;
  cmpp_d const * const dClosers[2] = {args->d->closer, dNoRows};

  if( !obBody || !sql ) goto cleanup;

  assert( dNoRows );
  //g_warn("args.argc=%d", args.argc);

#define nextArg (++i<args->argc ? args->argv[i] : 0)
  for( unsigned i = 1; i < args->argc; ++i ){
    cmpp_arg const * arg = args->argv[i];
    //g_warn("arg=%s <<%s>>", cmpp_tt_cstr(arg->ttype), arg->z);
    if( cmpp_arg_equals_c(arg, "define") ){
      if( seenDefine ){
        cmpp__dx_err_just_once(dx, arg);
        goto cleanup;
      }
      seenDefine = true;
      continue;
    }
    if( cmpp_arg_equals_c(arg, "emit") ){
      if( seenEmit ){
        cmpp__dx_err_just_once(dx, arg);
        goto cleanup;
      }
      seenEmit = true;
      continue;
    }
    if( cmpp_arg_equals_c(arg, "-chomp") ){
      ++nChomp;
      continue;
    }
    if( cmpp_arg_equals_c(arg, "-unchomp") ){
      ++nUnchomp;
      continue;
    }
    if( cmpp_arg_equals_c(arg, "-batch") ){
      if( batchMode ){
        cmpp__dx_err_just_once(dx, arg);
        goto cleanup;
      }
      batchMode = true;
      continue;
    }
    if( !sql->n ){
      if( !arg->n ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Expecting a non-empty SQL argument.");
        goto cleanup;
      }
      if( cmpp__consume_sql_args(pp, args, &i, &aBind) ){
        goto cleanup;
      }
      if( cmpp_arg_interpolate(pp, arg, sql, cmpp_arg_interpolate_BRACE_CALL) ){
        goto cleanup;
      }
      //g_warn("SQL: <<%s>>", sql->z);
      continue;
    }
    cmpp_errf(pp, CMPP_RC_MISUSE, "Unhandled arg: %s", arg->z);
    goto cleanup;
  }
#undef nextArg

  if( ppCode ) goto cleanup;
  seenEmit |= args->isDxCall;

  if( seenDefine || seenEmit ){
    if( seenDefine && nChomp ){
      /* fixme (2026-07-30): support chomp for define */
      serr("-chomp and define may not be used together.");
      goto cleanup;
    }else if( batchMode ){
      serr("-batch and define/emit may not be used together.");
      goto cleanup;
    }
  }
  if( !sql->n ){
    serr("Expecting an SQL-string argument.");
    goto cleanup;
  }

  if( batchMode ){
    if( aBind ){
      serr("Bindable emits may not be used with -batch.");
      goto cleanup;
    }
    char *zErr = 0;
    cmpp__pi(args->pp);
    int rc = sqlite3_exec(pi->db.dbh, (char const *)sql->z, 0, 0, &zErr);
    rc = cmpp__db_rc(args->pp, rc, zErr);
    sqlite3_free(zErr);
    goto cleanup;
  }

  if( cmpp__db_rc(pp, sqlite3_prepare_v2(
                    pp->pimpl->db.dbh, (char const *)sql->z,
                    (int)sql->n, &q, 0), 0) ){
    goto cleanup;
  }else if( !q ){
    cmpp_errf(pp, CMPP_RC_RANGE,
              "Empty SQL is not permitted.");
    goto cleanup;
  }
  //g_warn("SQL via stmt: <<%s>>", sqlite3_sql(q));
  int const nCol = sqlite3_column_count(q);
  if( !nCol ){
    cmpp_errf(pp, CMPP_RC_RANGE,
              "SQL does not have any result columns.");
    goto cleanup;
  }
  if( !seenDefine ){
    if( cmpp_sp_begin(pp) ) goto cleanup;
    spStarted = true;
  }

  if( aBind && cmpp__bind_group(pp, q, aBind) ){
    goto cleanup;
  }

  bool gotARow = false;
  cmpp_dx_pos dxPosStart;
  cmpp_flag32_t const consumeFlags = cmpp_dx_consume_F_PROCESS_OTHER_D;
  cmpp_dx_pos_save(dx, &dxPosStart);
  int const nChompOrig = nChomp;
  while( 0==ppCode ){
    int const dbrc = cmpp__step(pp, q, false);
    if( SQLITE_ROW==dbrc ){
      gotARow = true;
      nChomp = nChompOrig;
      cmpp_b_reuse(obBody);
      if( cmpp__define_from_row(pp, q, false,
                                seenEmit ? obBody : 0) ){
        break;
      }
      if( seenEmit ){
        while( nChomp-- && cmpp_b_chomp(obBody) ){}
        while( nUnchomp-- ) cmpp_b_append_ch(obBody, '\n');
        if( seenEmit && !obBody->errCode && obBody->n ){
          cmpp_out_raw(args->pp, obBody->z, obBody->n);
        }
        break;
      }else if( seenDefine ){
        break;
      }
      cmpp_dx_pos_restore(dx, &dxPosStart);
      /* If it weren't for -chomp, we wouldn't need to
         buffer this. */
      if( cmpp_dx_consume_b(dx, obBody, dClosers,
                            sizeof(dClosers)/sizeof(dClosers[0]),
                            consumeFlags) ){
        goto cleanup;
      }
      assert( dx->d == dClosers[0] || dx->d == dClosers[1] );
      while( nChomp-- && cmpp_b_chomp(obBody) ){}
      if( obBody->n && cmpp_out_raw(args->pp, obBody->z, obBody->n) ){
        break;
      }
      if( dx->d == dNoRows ){
        if( cmpp_dx_consume(dx, &obNull, dClosers, 1/*one!*/,
                            consumeFlags) ){
          goto cleanup;
        }
        assert( dx->d == dClosers[0] );
        /* TODO? chomp? */
      }
      continue;
    }
    if( 0==ppCode && (seenDefine || seenEmit) ){
      /* If we got here, there was no result row. */
      cmpp__define_from_row(pp, q, true, 0);
    }
    break;
  }/*result row loop*/
  cmpp__stmt_reset(q);
  if( ppCode ) goto cleanup;

  while( !seenDefine && !seenEmit && !gotARow ){
    /* No result rows. Skip past the body, emitting the #query:no-rows
       content, if any. We disable @token processing for that first
       step because (A) the output is not going anywhere, so no need
       to expand it (noting that expanding may have side effects via
       @[call...]@) and (B) the @tokens@ referring to this query's
       results will not have been set because there was no row to set
       them from, so @expanding@ them would fail. */
    cmpp_atpol_e const atpol = cmpp_atpol_get(args->pp);
    if( cmpp_atpol_set(args->pp, cmpp_atpol_OFF) ) break;
    cmpp_dx_consume(dx, &obNull, dClosers,
                    sizeof(dClosers)/sizeof(dClosers[0]),
                    consumeFlags);
    cmpp_atpol_set(args->pp, atpol);
    if( dxppCode ) break;
    assert( dx->d == dClosers[0] || dx->d == dClosers[1] );
    if( dx->d == dNoRows ){
      if( cmpp_dx_consume(dx, 0, dClosers, 1/*one!*/,
                          consumeFlags) ){
        break;
      }
      assert( dx->d == dClosers[0] );
      /* TODO? chomp? */
    }
    break;
  }

cleanup:
  cmpp_b_return(args->pp, obBody);
  cmpp_b_return(args->pp, sql);
  sqlite3_finalize(q);
  if( spStarted ) cmpp_sp_rollback(pp);
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
  cmpp_arg const * arg = dx->args.arg0;
  if( !dx->args.argc ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE, "Expecting an argument.");
    return;
  }
  //g_warn("argc=%d", dx->args.argc);
  cmpp_b * const b = cmpp_b_borrow(args->pp);
  for( ; arg && !cmpp_err_has(args->pp); arg = arg->next ){
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
      //assert(!cmpp_dx_is_eliding(args->dx));
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
  if( !dx->args.argc ){
    cmpp_dx_errf(dx, CMPP_RC_MISUSE,
                    "%s%s expects ?flags? ...args",
                    cmpp_dx_delim(dx), args->d->name.z);
    goto end;
  }
  cmpp_b_append_ch(bSep, ' ');
  cmpp_check_oom(args->pp, bSep->z);
  for( cmpp_arg const * arg = dx->args.arg0; arg
         && !b->errCode
         && !bSep->errCode
         && !cmpp_err_has(args->pp);
       arg = arg->next ){
#define FLAG(X)if( cmpp_arg_isflag_c(arg, X) )
    FLAG("-s"){
      if( !arg->next ){
        cmpp_errf(args->pp, CMPP_RC_MISUSE,
                  "Missing SEPARATOR argument to -s.");
        break;
      }
      cmpp_arg_interpolate(args->pp, arg->next,
                    cmpp_b_reuse(bSep),
                    cmpp_arg_interpolate_BRACE_CALL);
      arg = arg->next;
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
  if( !bL || !!bR ) goto end;
  for( cmpp_arg const * arg = args->dx->args.arg0; arg
         && !cmpp_err_has(args->pp);
       arg = arg->next ){
    if( !bL->z ){
      cmpp_arg_interpolate(args->pp, arg, bL, a2bFlags);
      continue;
    }
    if( !bR->z ){
      cmpp_arg_interpolate(args->pp, arg, bR, a2bFlags);
      continue;
    }
    break;
  }

  if( cmpp_err_has(args->pp) ) goto end;
  if( !bL->z || !bR->z ){
    cmpp_errf(args->pp, CMPP_RC_MISUSE, "Usage: LHS RHS");
    goto end;
  }
  assert( bL->z );
  assert( bR->z );
  char cbuf[20];
  int const cmp = strcmp((char*)bL->z, (char*)bR->z);
  int const n = snprintf(cbuf, sizeof(cbuf), "%d", cmp);
  assert(n>0);
  cmpp_out_raw(args->pp, cbuf, (cmpp_size_t)n);

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
/* …（同文件无关代码省略）… */
int cmpp__d_delayed_load(cmpp *pp, char const *zName){
  if( ppCode ) return ppCode;
  int rc = CMPP_RC_NO_DIRECTIVE;
  unsigned const nName = strlen(zName);

  pp->pimpl->flags.isInternalDirectiveReg = true;

#define M(NAME) (nName==sizeof(NAME)-1 && 0==strcmp(zName,NAME))
#define M_OC(NAME) (M(NAME) || M("/" NAME))
#define M_IF(NAME) if( M(NAME) )
#define CF(X) cmpp_f_F_ ## X
#define F_A_RAW CF(ARGS_RAW)
#define F_A_V4 CF(ARGS_V4)
#define F_A_LIST CF(ARGS_LIST)
#define F_EXPR CF(ARGS_LIST) | CF(NOT_SIMPLIFY)
#define F_UNSAFE cmpp_f_F_NOT_IN_SAFEMODE
#define F_NC cmpp_f_F_NO_CALL
#define F_CALL cmpp_f_F_CALL_ONLY
#define DREG0(SYMNAME, NAME, OPENER, OFLAGS, CLOSER, CFLAGS) \
  cmpp_d_reg SYMNAME = {  \
    .name = NAME,         \
    .opener = {           \
      .f = OPENER,        \
      .flags = OFLAGS     \
    },                    \
    .closer = {           \
      .f = CLOSER,        \
      .flags = CFLAGS     \
    },                    \
    .dtor = 0,            \
    .state = 0,           \
    .arity = -1,          \
    .usage = "#directive .usage is TODO. See README.md for docs." \
  }

#define DREG(NAME, OPENER, OFLAGS, CLOSER, CFLAGS )         \
  DREG0(const rReg, NAME, OPENER, OFLAGS, CLOSER, CFLAGS ); \
  rc = cmpp_d_register(pp, &rReg, NULL);                    \
  goto end

  /* The #if family requires some hand-holding... */
  if( M_OC("if") || M("elif") || M("else") ) {
    DREG0(rIf,   "if",
          cmpp_f_if, F_EXPR | F_NC | CF(FLOW_CONTROL),
          cmpp_f_if_dangler, 0);
    DREG0(rElif, "elif",
          cmpp_f_if_dangler, F_NC,
          0, 0);
    DREG0(rElse, "else",
          cmpp_f_if_dangler, F_NC,
          0, 0);
    CmppIfState * const cis = cmpp_malloc2(pp, sizeof(*cis));
    if( !cis ) goto end;
    memset(cis, 0, sizeof(*cis));
    rIf.state = cis;
    rIf.dtor  = cmpp_mfree;
    if( cmpp_d_register(pp, &rIf, &cis->dIf)
        /* rIf must be first to avoid leaking cis on error */
        || cmpp_d_register(pp, &rElif, &cis->dElif)
        || cmpp_d_register(pp, &rElse, &cis->dElse) ){
      rc = ppCode;
    }else{
      assert( cis->dIf && cis->dElif && cis->dElse );
      assert( !cis->dEndif );
      assert( cis == cis->dIf->impl.state );
      assert( cmpp_mfree==cis->dIf->impl.dtor );
      cis->dElif->impl.state
        = cis->dElse->impl.state
        = cis;
      cis->dElif->closer
        = cis->dElse->closer
        = cis->dEndif
        = cis->dIf->closer;
      rc = 0;
    }
    goto end;
  }/* #if and friends */

  /* Basic core directives... */
#define M_IF_CORE(N,OPENER,OFLAGS,CLOSER,CFLAGS)  \
  if( M_OC(N) ){                                  \
    DREG(N, OPENER, OFLAGS, CLOSER, CFLAGS);      \
  } (void)0

  M_IF_CORE("@",                cmpp_f_at,           F_A_LIST,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("arg",              cmpp_f_arg,          F_A_LIST, 0, 0);
  M_IF_CORE("assert",           cmpp_f_expr,         F_EXPR, 0, 0);
  M_IF_CORE("base64",           cmpp_f_base64,       F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("cmp",              cmpp_f_cmp,          F_A_LIST, 0, 0);
  M_IF_CORE("define",           cmpp_f_define,       F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("delimiter",        cmpp_f_delimiter,    F_A_LIST,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("error",            cmpp_f_error,        F_A_RAW, 0, 0);
  M_IF_CORE("eval",             cmpp_f_eval,         F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("expand",           cmpp_f_expand,       F_A_V4,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("expr",             cmpp_f_expr,         F_EXPR, 0, 0);
  M_IF_CORE("join",             cmpp_f_join,         F_A_LIST, 0, 0);
  M_IF_CORE("once",             cmpp_f_once,         F_A_LIST | F_NC,
                                cmpp_f_dangling_closer, 0);
  M_IF_CORE("pragma",           cmpp_f_pragma,       F_A_V4, 0, 0);
  M_IF_CORE("savepoint",        cmpp_f_savepoint,    F_A_V4, 0, 0);
  M_IF_CORE("stderr",           cmpp_f_stderr,       F_A_RAW, 0, 0);
  M_IF_CORE("sum",              cmpp_f_sum,          F_A_V4, 0, 0);
  M_IF_CORE("undef",            cmpp_f_undef,        F_A_V4, 0, 0);
  M_IF_CORE("undefined-policy", cmpp_f_undef_policy, F_A_V4, 0, 0);
  M_IF_CORE("//",               cmpp_f_noop,         F_A_RAW, 0, 0);
  M_IF_CORE("file",             cmpp_f_file,
                                F_A_V4 | F_UNSAFE, 0, 0);

#undef M_IF_CORE


  /* Directives which can be disabled via build flags or
   flags to cmpp_ctor()... */
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


#ifndef CMPP_OMIT_D_DB
  M_IF_FLAGGED("attach", cmpp_ctor_F_NO_DB,
               cmpp_f_attach, F_A_V4 | F_UNSAFE,
               0, 0);
  M_IF_FLAGGED("detach", cmpp_ctor_F_NO_DB,
               cmpp_f_detach, F_A_V4 | F_UNSAFE,
               0, 0);
  if( 0==(cmpp_ctor_F_NO_DB & pp->pimpl->flags.ctorFlags)
      && (M_OC("query") || M("query:no-rows")) ){
    DREG0(rQ, "query", cmpp_f_query, F_A_V4 | F_UNSAFE,
          cmpp_f_dangling_closer, 0);
    cmpp_d * dQ = 0;
    rc = cmpp_d_register(pp, &rQ, &dQ);
    if( rc ) goto end;
    /*
      It would be preferable to delay registration of query:no-rows
      until we need it, but doing so causes an error when:

      |#if 0
      |#query
      |...
      |#query:no-rows   HERE
      |...
      |#/query
      |#/if

      Because query:no-rows won't have been registered, and unknown
      directives are an error even in skip mode. Maybe they shouldn't
      be. Maybe we should just skip them in skip mode.  That's only
      been an issue since doing delayed registration of directives, so
      it's not come up until recently (as of 2025-10-27). i was so
      hoping to be able to get _rid_ of skip mode at some point.
    */
    cmpp_d * dNoRows = 0;
    cmpp_d_reg const rNR = {
      .name = "query:no-rows",
      .arity = -1,
      .opener = {
        .f = cmpp_f_dangling_closer,
        .flags = F_NC
      }
    };
    rc = cmpp_d_register(pp, &rNR, &dNoRows);
    if( rc ) goto end;
    dNoRows->closer = dQ->closer;
    assert( !dQ->impl.state );
    dQ->impl.state = dNoRows;
    goto end;
  }
#endif /*CMPP_OMIT_D_DB*/


#undef M_IF_FLAGGED

#ifndef NDEBUG
  M_IF("experiment"){
    DREG("experiment", cmpp_f_experiment,
         F_A_V4 | F_UNSAFE, 0, 0);
  }
#endif

end:
#undef DREG
#undef DREG0
#undef F_EXPR
#undef F_A_RAW
#undef F_A_LIST
#undef F_A_V4
#undef F_UNSAFE
#undef F_NC
#undef F_CALL
#undef CF
#undef M
#undef M_OC
#undef M_IF
  pp->pimpl->flags.isInternalDirectiveReg = false;
  return ppCode ? ppCode : rc;
}
/* …（同文件无关代码省略）… */
bool cmpp__file_is_readable(char const *zFile){
  return 0==access(zFile, R_OK);
}
/* …（同文件无关代码省略）… */
#define cmpp_argOps_cmp_map(E) E(Eq) E(Neq) E(Lt) E(Le) E(Gt) E(Ge)
/* …（同文件无关代码省略）… */
cmpp_argOp const * cmpp_argOp_for_tt(cmpp_tt_e tt){
  switch(tt){
    case cmpp_tt_OpAnd:     return &cmpp_argOps.opAnd;
    case cmpp_tt_OpOr:      return &cmpp_argOps.opOr;
    case cmpp_tt_OpGlob:     return &cmpp_argOps.opGlob;
    case cmpp_tt_OpNot:     return &cmpp_argOps.opNot;
    case cmpp_tt_OpDefined: return &cmpp_argOps.opDefined;
#define E(NAME) case cmpp_tt_OpCmp ## NAME: return &cmpp_argOps.op ## NAME;
  cmpp_argOps_cmp_map(E)
#undef E
    default: return NULL;
  }
}
#define argOp(ARG) cmpp_argOp_for_tt((ARG)->ttype)
/* …（同文件无关代码省略）… */
static void cmpp_argOp__cmp_bind(cmpp * const pp,
                                 sqlite3_stmt * const q,
                                 int bindNdx,
                                 cmpp_arg const ** paArg){
  cmpp_arg const * const arg = *paArg;
  assert(arg);
  switch( ppCode ? 0 : arg->ttype ){
    case 0: break;
    case cmpp_tt_Word:
      /* In this case, q is supposed to be set up to use
         CMPP__SEL_V_FROM(bindNdx), i.e. it expects the verbatim word
         and performs the expansion to its value in the query. */
      cmpp__bind_textn(pp, q, bindNdx, arg->z, arg->n);
      *paArg = arg->next;
      break;
    case cmpp_tt_StringBT:
    case cmpp_tt_StringDQ:
    case cmpp_tt_StringSQ:
    case cmpp_tt_IntDec:{
      cmpp__bind_arg(pp, q, bindNdx, arg);
      *paArg = arg->next;
      break;
    }
    case cmpp_tt_OpNot:
    case cmpp_tt_OpDefined:
    case cmpp_tt_GroupParen:{
      int rv = 0;
      if( 0==cmpp__arg_toBool(pp, arg, &rv, paArg) ){
        cmpp__bind_int(pp, q, bindNdx, rv);
      }
      *paArg = arg->next;
      break;
    }
      /* TODO? cmpp_tt_GroupParen */
    default:
      cmpp_errf(pp, CMPP_RC_TYPE,
                   "Invalid argument type (%s) for the comparison "
                   "queries: %s",
                   cmpp_tt_cstr(arg->ttype), arg->z);
  }
}
/* …（同文件无关代码省略）… */
static void cmpp_argOp__cmp_apply(cmpp * const pp,
                                 cmpp_argOp const * const op,
                                 sqlite3_stmt * const q,
                                 int * const pResult){
  if( 0==ppCode ){
    int rc = cmpp__step(pp, q, false);
    assert( SQLITE_ROW==rc || ppCode );
    if( SQLITE_ROW==rc ){
      rc = sqlite3_column_int(q, 0);
    }else{
      rc = 0;
    }
    switch( op->ttype ){
      case 0: break;
      case cmpp_tt_OpCmpEq:  *pResult = 0==rc; break;
      case cmpp_tt_OpCmpNeq: *pResult = 0!=rc; break;
      case cmpp_tt_OpCmpLt:  *pResult = rc<0;  break;
      case cmpp_tt_OpCmpLe:  *pResult = rc<=0; break;
      case cmpp_tt_OpCmpGt:  *pResult = rc>0;  break;
      case cmpp_tt_OpCmpGe:  *pResult = rc>=0; break;
      default:
        cmpp__fatal("Cannot happen: invalid arg mapping");
    }
  }
  cmpp__stmt_reset(q);
}

/**
/* …（同文件无关代码省略）… */
static void cmpp_argOp_applyTo(cmpp *pp,
                               cmpp_argOp const * const op,
                               int lhs,
                               cmpp_arg const ** paRhs,
                               int * pResult){
  sqlite3_stmt * q = 0;
  cmpp_arg const * aRhs = *paRhs;
  assert(aRhs);
  q = cmpp_tt_Word==aRhs->ttype
    ? cmpp__stmt(pp, CmppStmt_cmpVD, false)
    : cmpp__stmt(pp, CmppStmt_cmpVV, false);
  if( q ){
    char numbuf[32];
    int const nNum = snprintf(numbuf, sizeof(numbuf), "%d", lhs);
    cmpp__bind_textn(pp, q, 1, ustr_c(numbuf), nNum);
    cmpp_argOp__cmp_bind(pp, q, 2, paRhs);
    cmpp_argOp__cmp_apply(pp, op, q, pResult);
  }
}
/* …（同文件无关代码省略）… */
    q = cmpp__stmt(pp, CmppStmt_cmpVV, false);
  }
  if( q ){
    //cmpp__bind_textn(pp, q, 1, vLhs->z, vLhs->n);
    cmpp_argOp__cmp_bind(pp, q, 1, &vLhs);
    cmpp_argOp__cmp_bind(pp, q, 2, pvRhs);
    cmpp_argOp__cmp_apply(pp, op, q, pResult);
/* …（同文件无关代码省略）… */
int cmpp__arg_evalSubToInt(cmpp *pp,
                           cmpp_arg const *arg,
                           int * pResult){
  cmpp_args sub = cmpp_args_empty;
  if( 0==cmpp_args_parse(pp, &sub, arg->z, arg->n, 0) ){
    cmpp__args_evalToInt(pp, &sub, pResult);
  }
  cmpp_args_cleanup(&sub);
  return ppCode;
}
/* …（同文件无关代码省略）… */
int cmpp__args_evalToInt(cmpp * const pp,
                         cmpp_args const *pArgs,
                         int * pResult){
  if( ppCode ) return ppCode;

  cmpp_arg const * pNext = 0;
  cmpp_arg const * pPrev = 0;
  int result = *pResult;
  cmpp_b osL = cmpp_b_empty;
  cmpp_b osR = cmpp_b_empty;
  static int level = 0;
  ++level;

#define lout(fmt,...) if(0) g_stderr("%.*c" fmt, level*2, ' ', __VA_ARGS__)

  //lout("START %s(): %s\n", __func__, pArgs->pimpl->buf.argsRaw.z);
  for( cmpp_arg const *arg = pArgs->arg0;
       arg && 0==ppCode;
       pPrev = arg, arg = pNext ){
    pNext = arg->next;
    if( cmpp_tt_Noop==arg->ttype ){
      arg = pPrev /* help the following arg to DTRT */;
      continue;
    }
    cmpp_argOp const * const thisOp = argOp(arg);
    cmpp_argOp const * const nextOp = pNext ? argOp(pNext) : 0;
    if( 0 ){
      lout("arg: %s @%p %s\n",
           cmpp__tt_cstr2(arg->ttype, true), arg, arg->z);
      if(1){
        if( pPrev ) lout("  prev arg: %s %s\n",
                         cmpp__tt_cstr2(pPrev->ttype, true), pPrev->z);
        if( pNext ) lout("  next arg: %s %s\n",
                         cmpp__tt_cstr2(pNext->ttype, true), pNext->z);
      }
    }
    if( thisOp ){ /* Basic validation */
      if( !pNext ){
        serr("Missing '%s' RHS.",
             cmpp__tt_cstr2(thisOp->ttype, false));
        break;
      }else if( !pPrev && 2==thisOp->arity  ){
        serr("Missing %s LHS.",
             cmpp__tt_cstr2(thisOp->ttype, false));
        break;
      }
      if( nextOp && nextOp->arity>1 ){
        serr("Invalid '%s' RHS: %s", arg->z, pNext->z);
        break;
      }
    }

    switch( arg->ttype ){

      case cmpp_tt_OpNot:
      case cmpp_tt_OpDefined:
        if( pPrev && !argOp(pPrev) ){
          cmpp_errf(pp, CMPP_RC_CANNOT_HAPPEN,
                       "We expected to have consumed '%s' by "
                       "this point.",
                       pPrev->z);
        }else{
          cmpp__arg_toBool(pp, arg, &result, &pNext);
        }
        break;

      case cmpp_tt_OpAnd:
      case cmpp_tt_OpOr:{
        assert( pNext );
        assert( pPrev );
        /* Reminder to self: we can't add short-circuiting of the RHS
           right now because the handling of chained unary ops on the
           RHS is handled via cmpp__arg_toBool(). */
        int rv = 0;
        if( 0==cmpp__arg_toBool(pp, pNext, &rv, &pNext) ){
          if( cmpp_tt_OpAnd==arg->ttype ) result = result && rv;
          else result = result || rv;
        }
        //g_warn("post-and/or pNext=%s\n", pNext ? pNext->z : 0);
        break;
      }

      case cmpp_tt_OpNotGlob:
      case cmpp_tt_OpGlob:{
        assert( pNext );
        assert( pPrev );
        assert( pNext!=arg );
        assert( pPrev!=arg );
        if( cmpp_arg_interpolate(pp, pNext, &osL, 0) ){
          break;
        }
        unsigned char const * const zGlob = osL.z;
        if( 0==cmpp_arg_interpolate(pp, pPrev, &osR, 0) ){
          if( 0 ){
            g_warn("zGlob=[%s] z=[%s]", zGlob, osR.z);
          }
          result = 0==sqlite3_strglob((char const *)zGlob,
                                      (char const *)osR.z);
          if( cmpp_tt_OpNotGlob==arg->ttype ){
            result = !result;
          }
          //g_warn("\nzGlob=%s\nz=%s\nresult=%d", zGlob, z, result);
        }
        pNext = pNext->next;
        break;
      }

#define E(NAME) case cmpp_tt_OpCmp ## NAME:
      cmpp_argOps_cmp_map(E) {
        cmpp_argOp const * const prevOp = pPrev ? argOp(pPrev) : 0;
        if( prevOp ){
          /* Chained operators */
          cmpp_argOp_applyTo(pp, thisOp, result, &pNext, &result);
        }else{
          assert( pNext );
          assert( pPrev );
          assert( thisOp );
          assert( thisOp->xCall );
          thisOp->xCall(pp, thisOp, pPrev, &pNext, &result);
        }
        break;
      }
#undef E

#define checkConsecutiveNonOps                             \
      if( pPrev && !argOp(pPrev) ){                        \
        serr("Illegal consecutive non-operators: %s %s",   \
             pPrev->z, arg->z);                            \
        break;                                             \
      }(void)0

      case cmpp_tt_IntDec:
      case cmpp_tt_StringBT:
      case cmpp_tt_StringDQ:
      case cmpp_tt_StringSQ:
        checkConsecutiveNonOps;
        if( !cmpp_is_int(arg->z, arg->n, &result) ){
          /* This is mostly for and/or ops. glob will reach back and
             grab arg->z. */
          result = 0;
        }
        break;
      case cmpp_tt_Word:
        checkConsecutiveNonOps;
        cmpp__get_int(pp, arg->z, arg->n, &result);
        break;
      case cmpp_tt_GroupParen:{
        checkConsecutiveNonOps;
        cmpp_args sub = cmpp_args_empty;
        if( 0==cmpp_args_parse(pp, &sub, arg->z, arg->n, 0) ){
          cmpp__args_evalToInt(pp, &sub, &result);
        }
        cmpp_args_cleanup(&sub);
        break;
      }
      case cmpp_tt_GroupBrace:{
        checkConsecutiveNonOps;
        cmpp_b b = cmpp_b_empty;
        if( 0==cmpp_call_str(pp, arg->z, arg->n, &b, 0) ){
          cmpp_is_int(b.z, b.n, &result);
        }
        cmpp_b_clear(&b);
        break;
      }
#undef checkConsecutiveNonOps
      default:
        assert( arg->z );
        serr("Illegal expression token %s: %s",
             cmpp__tt_cstr2(arg->ttype, true), arg->z);
    }/*switch(arg->ttype)*/
  }/* foreach arg */
  if( 0 ){
    lout("END   %s() result=%d\n",  __func__, result);
  }
  --level;
  if( !ppCode ){
    *pResult = result;
  }
  cmpp_b_clear(&osL);
  cmpp_b_clear(&osR);
  return ppCode;
#undef lout
}
/* …（同文件无关代码省略）… */
int cmpp__arg_toBool(cmpp * const pp, cmpp_arg const *arg,
                    int * pResult, cmpp_arg const **pNext){
  switch( ppCode ? 0 : arg->ttype ){
    case 0: break;

    case cmpp_tt_Word:
      *pNext = arg->next;
      *pResult = cmpp__get_bool(pp, arg->z, arg->n);
      break;

    case cmpp_tt_IntDec:
      *pNext = arg->next;
      cmpp_is_int(arg->z, arg->n, pResult)/*was already validated*/;
      break;

    case cmpp_tt_StringBT:
    case cmpp_tt_StringDQ:
    case cmpp_tt_StringSQ:{
      int rc = 0;
      *pNext = arg->next;
      if( cmpp_is_int(arg->z, arg->n, &rc) ){
        *pResult = rc;
      }else{
        *pResult = arg->n>0 && 0!=memcmp("0\0", arg->z, 2);
      }
      break;
    }

    case cmpp_tt_GroupParen:{
      *pNext = arg->next;
      cmpp_args sub = cmpp_args_empty;
      if( 0==cmpp_args_parse(pp, &sub, arg->z, arg->n, 0) ){
        cmpp__args_evalToInt(pp, &sub, pResult);
      }
      cmpp_args_cleanup(&sub);
      break;
    }

    case cmpp_tt_OpDefined:
      if( !arg->next ){
        serr("Missing '%s' RHS.", arg->z);
      }else if( cmpp_tt_Word!=arg->next->ttype ){
        serr( "Invalid '%s' RHS: %s", arg->z, arg->next->z);
      }else{
        cmpp_arg const * aOperand = arg->next;
        *pNext = aOperand->next;
        if( aOperand->n>1
            && '#'==aOperand->z[0]
            && !!cmpp__d_search3(pp, (char const*)aOperand->z+1,
                                 cmpp__d_search3_F_NO_DLL) ){
          *pResult = 1;
        }else{
          *pResult = cmpp__has(pp, (char const *)aOperand->z,
                               aOperand->n);
        }
      }
      break;

    case cmpp_tt_OpNot:{
      assert( arg->next && "See cmpp_args__not_simplify()");
      assert( cmpp_tt_OpNot!=arg->next->ttype && "See cmpp_args__not_simplify()");
      if( 0==cmpp__arg_toBool(pp, arg->next, pResult, pNext) ){
        *pResult = !*pResult;
      }
      break;
    }

    default:
      serr("Invalid token type %s for %s(): %s",
           cmpp__tt_cstr2(arg->ttype, true), __func__, arg->z);
      break;
  }
  return ppCode;
}
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
      cmpp__bind_textn(pp, q, bindNdx, arg->z, (int)arg->n);
      break;

    case cmpp_tt_Word:{
      cmpp_b os = cmpp_b_empty;
      if( 0==cmpp_arg_interpolate(pp, arg, &os, 0) ){
        if( 0 ){
          g_warn("bind #%d <<%s>> => <<%.*s>>",
                 bindNdx, arg->z, (int)os.n, os.z);
        }
        cmpp__bind_textn(pp, q, bindNdx, os.z, (int)os.n);
      }
      cmpp_b_clear(&os);
      break;
    }

    case cmpp_tt_GroupParen:{
      cmpp_args sub = cmpp_args_empty;
      int i = 0;
      if( 0==cmpp_args_parse(pp, &sub, arg->z, arg->n, 0)
          && 0==cmpp__args_evalToInt(pp, &sub, &i) ){
        /* See comment above about cmpp_tt_Int. */
        cmpp__bind_int_text(pp, q, bindNdx, i);
      }
      cmpp_args_cleanup(&sub);
      break;
    }

    case cmpp_tt_GroupBrace:{
      cmpp_b b = cmpp_b_empty;
      cmpp_call_str(pp, arg->z, arg->n, &b, 0);
      cmpp__bind_textn(pp, q, bindNdx, b.z, b.n);
      cmpp_b_clear(&b);
      break;
    }

  }
  return ppCode;
}
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
#  define CMPP_D_MODULE 1
/* …（同文件无关代码省略）… */
static int cmpp__err_no_dlls(cmpp * const pp){
  return cmpp_errf(pp, CMPP_RC_UNSUPPORTED,
                      "No dlopen() equivalent is installed "
                      "for this build configuration.");
}
/* …（同文件无关代码省略）… */
int cmpp_module_load(cmpp * pp, char const * fname,
                     char const * symName){
  (void)fname; (void)symName;
  return cmpp__err_no_dlls(pp);
}
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
int cmpp__tt_for_group_char(int ch){
  switch(ch){
    case '{': return cmpp_tt_GroupSquiggly;
    case '[': return cmpp_tt_GroupBrace;
    case '(': return cmpp_tt_GroupParen;
    case '"': return cmpp_tt_StringDQ;
    case '\'': return cmpp_tt_StringSQ;
    case '`': return cmpp_tt_StringBT;
    default:
      return 0;
  }
}
/* …（同文件无关代码省略）… */
  return z;
}

/**
   Creates a new sqlite3_str populated with source location info from
   itch->pimpl->tz and tok (which must come from that tokenizer).
   Returns NULL only on OOM. Ownership of the returned value is
   transfered to the caller.
*/
CMPP__EXPORT(sqlite3_str *, cmpp_tizer_err_prefix)(cmpp_tizer const *tz,
                                                   unsigned char const *z){
  sqlite3_str * const str = cmpp__sqlite3_str_new(0);
  if( !str ) return 0;
  if( !z ) z = cmpp_tizer_errpos(tz);
  unsigned char const *zB = 0;
  unsigned char const *zE = 0;
  cmpp_tizer_full_range(tz, &zB, &zE);
  assert( z>=zB && z<=zE && "Else internal misuse of this API" );
  if( z>=zB && z<=zE ){
    cmpp_size_t col = 0;
    cmpp_size_t const ln = 1+cmpp_count_nl(zB, zE, z, &col);
    char const * zName = cmpp_tizer_name(tz);
    if( !zName ) zName = "<unnamed script>";
    sqlite3_str_appendf(str, "Script error in [%s], possibly near %d:%d: ",
                        zName, (int)ln, (int)col);
  }
  return str;
}
/* …（同文件无关代码省略）… */
CMPP__EXPORT(void, cmpp_tizer_errpos_set)(cmpp_tizer *tz,
                                          unsigned char const *z){
  if( z ){
    unsigned char const * zB = 0;
    unsigned char const * zE = 0;
    cmpp_tizer_full_range(tz, &zB, &zE);
    if( z>=zB && zE<=z ){
      tz->errToken = cmpp_token_empty;
      tz->errToken.z = tz->errToken.zInner = z;
    }/*else{
/* …（同文件无关代码省略）… */
    }
  }
  cmpp_ctor_opt const cfg = {
    .flags = ctorFlags
  };
  rc = cmpp_ctor(&pp, &cfg);
  if( rc ) goto end;
/* …（同文件无关代码省略）… */
#if defined(CMPP_MAIN_AUTOLOADER)
  {
    extern int CMPP_MAIN_AUTOLOADER(cmpp*,char const *,void*);
    cmpp_d_autoloader al = cmpp_d_autoloader_empty;
    al.f = CMPP_MAIN_AUTOLOADER;
    cmpp_d_autoloader_set(pp, &al);
  }
