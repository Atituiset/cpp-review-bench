// AUTO-DRAFT from postgres/postgres PR #1d657c6e013e152ed37b42a22ee9f291179ee50f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
#define SIGLENBIT(siglen) ((siglen) * BITS_PER_BYTE)
/* …（同文件无关代码省略）… */
typedef char *BITVECP;
/* …（同文件无关代码省略）… */
#define GETBYTE(x,i) ( *( (BITVECP)(x) + (int)( (i) / BITS_PER_BYTE ) ) )
/* …（同文件无关代码省略）… */
#define SETBIT(x,i)   GETBYTE(x,i) |=  ( 0x01 << ( (i) % BITS_PER_BYTE ) )
/* …（同文件无关代码省略）… */
#define HASHVAL(val, siglen) (((unsigned int)(val)) % SIGLENBIT(siglen))
#define HASH(sign, val, siglen) SETBIT((sign), HASHVAL(val, siglen))
/* …（同文件无关代码省略）… */
#define GETENTRY(vec,pos) ((SignTSVector *) DatumGetPointer((vec)->vector[(pos)].key))
/* …（同文件无关代码省略）… */
typedef struct
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int32		flag;
	char		data[FLEXIBLE_ARRAY_MEMBER];
} SignTSVector;
/* …（同文件无关代码省略）… */
#define ARRKEY		0x01
#define SIGNKEY		0x02
#define ALLISTRUE	0x04
/* …（同文件无关代码省略）… */
#define ISARRKEY(x) ( ((SignTSVector*)(x))->flag & ARRKEY )
/* …（同文件无关代码省略）… */
#define ISALLTRUE(x)	( ((SignTSVector*)(x))->flag & ALLISTRUE )
/* …（同文件无关代码省略）… */
#define GTHDRSIZE	( VARHDRSZ + sizeof(int32) )
#define CALCGTSIZE(flag, len) ( GTHDRSIZE + ( ( (flag) & ARRKEY ) ? ((len)*sizeof(int32)) : (((flag) & ALLISTRUE) ? 0 : (len)) ) )
/* …（同文件无关代码省略）… */
#define GETSIGN(x)	( (BITVECP)( (char*)(x)+GTHDRSIZE ) )
/* …（同文件无关代码省略）… */
#define GETARR(x)	( (int32*)( (char*)(x)+GTHDRSIZE ) )
#define ARRNELEM(x) ( ( VARSIZE(x) - GTHDRSIZE )/sizeof(int32) )
/* …（同文件无关代码省略）… */
static void
makesign(BITVECP sign, SignTSVector *a, int siglen)
{
	int32		k,
				len = ARRNELEM(a);
	int32	   *ptr = GETARR(a);

	MemSet(sign, 0, siglen);
	for (k = 0; k < len; k++)
		HASH(sign, ptr[k], siglen);
}
/* …（同文件无关代码省略）… */
static SignTSVector *
gtsvector_alloc(int flag, int len, BITVECP sign)
{
	int			size = CALCGTSIZE(flag, len);
	SignTSVector *res = palloc(size);

	SET_VARSIZE(res, size);
	res->flag = flag;

	if ((flag & (SIGNKEY | ALLISTRUE)) == SIGNKEY && sign)
		memcpy(GETSIGN(res), sign, len);

	return res;
}
/* …（同文件无关代码省略）… */
typedef struct
{
	bool		allistrue;
	BITVECP		sign;
} CACHESIGN;
/* …（同文件无关代码省略）… */
static void
fillcache(CACHESIGN *item, SignTSVector *key, int siglen)
{
	item->allistrue = false;
	if (ISARRKEY(key))
		makesign(item->sign, key, siglen);
	else if (ISALLTRUE(key))
		item->allistrue = true;
	else
		memcpy(item->sign, GETSIGN(key), siglen);
}
/* …（同文件无关代码省略）… */
typedef struct
{
	OffsetNumber pos;
	int32		cost;
} SPLITCOST;
/* …（同文件无关代码省略）… */
		seed_2 = 2;
	}

	/* form initial .. */
	datum_l = gtsvector_alloc(SIGNKEY | (cache[seed_1].allistrue ? ALLISTRUE : 0),
							  siglen, cache[seed_1].sign);
	datum_r = gtsvector_alloc(SIGNKEY | (cache[seed_2].allistrue ? ALLISTRUE : 0),
							  siglen, cache[seed_2].sign);
	union_l = GETSIGN(datum_l);
	union_r = GETSIGN(datum_r);
	maxoff = OffsetNumberNext(maxoff);
	fillcache(&cache[maxoff], GETENTRY(entryvec, maxoff), siglen);
	/* sort before ... */
	costvector = palloc_array(SPLITCOST, maxoff);
	for (j = FirstOffsetNumber; j <= maxoff; j = OffsetNumberNext(j))
