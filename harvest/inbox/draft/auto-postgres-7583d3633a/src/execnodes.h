// AUTO-DRAFT from postgres/postgres PR #303c9b36205d11e41c7d5d61a1802ec50bccbf5a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

typedef struct ExprState ExprState;
typedef struct ExprContext ExprContext;
/* …（同文件无关代码省略）… */
typedef struct TupleTableSlot TupleTableSlot;
/* …（同文件无关代码省略）… */
typedef struct TupleHashTableData *TupleHashTable;
/* …（同文件无关代码省略）… */
typedef struct TupleHashTableData
{
	tuplehash_hash *hashtab;	/* underlying simplehash hash table */
	int			numCols;		/* number of columns in lookup key */
	AttrNumber *keyColIdx;		/* attr numbers of key columns */
	ExprState  *tab_hash_expr;	/* ExprState for hashing table datatype(s) */
	ExprState  *tab_eq_func;	/* comparator for table datatype(s) */
	Oid		   *tab_collations; /* collations for hash and comparison */
	MemoryContext tuplescxt;	/* memory context storing hashed tuples */
	MemoryContext tempcxt;		/* context for function evaluations */
	Size		additionalsize; /* size of additional data */
	TupleTableSlot *tableslot;	/* slot for referencing table entries */
	/* The following fields are set transiently for each table search: */
	TupleTableSlot *inputslot;	/* current input tuple's slot */
	ExprState  *in_hash_expr;	/* ExprState for hashing input datatype(s) */
	ExprState  *cur_eq_func;	/* comparator for input vs. table */
	ExprContext *exprcontext;	/* expression context */
} TupleHashTableData;
/* …（同文件无关代码省略）… */
	TupleHashTable hashnulls;	/* hash table for rows with null(s) */
	bool		havehashrows;	/* true if hashtable is not empty */
	bool		havenullrows;	/* true if hashnulls is not empty */
	MemoryContext tuplesContext;	/* context containing hash tables' tuples */
	ExprContext *innerecontext; /* econtext for computing inner tuples */
	int			numCols;		/* number of columns being hashed */
