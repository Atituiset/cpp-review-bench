// AUTO-DRAFT from postgres/postgres PR #3ba886b35a2804dcd275868a19135a31ceb81386
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
typedef unsigned int pg_wchar;
/* …（同文件无关代码省略）… */
/*
 * pg_wchar stuff
 */
typedef int (*mb2wchar_with_len_converter) (const unsigned char *from,
											pg_wchar *to,
											int len);

typedef int (*wchar2mb_with_len_converter) (const pg_wchar *from,
											unsigned char *to,
											int len);

typedef int (*mblen_converter) (const unsigned char *mbstr);

typedef int (*mbdisplaylen_converter) (const unsigned char *mbstr);

typedef bool (*mbcharacter_incrementer) (unsigned char *mbstr, int len);

typedef int (*mbchar_verifier) (const unsigned char *mbstr, int len);

typedef int (*mbstr_verifier) (const unsigned char *mbstr, int len);

typedef struct
{
	mb2wchar_with_len_converter mb2wchar_with_len;	/* convert a multibyte
													 * string to a wchar */
	wchar2mb_with_len_converter wchar2mb_with_len;	/* convert a wchar string
													 * to a multibyte */
	mblen_converter mblen;		/* get byte length of a char */
	mbdisplaylen_converter dsplen;	/* get display width of a char */
	mbchar_verifier mbverifychar;	/* verify multibyte character */
	mbstr_verifier mbverifystr; /* verify multibyte string */
	int			maxmblen;		/* max bytes for a char in this encoding */
} pg_wchar_tbl;

extern PGDLLIMPORT const pg_wchar_tbl pg_wchar_table[];
/* …（同文件无关代码省略）… */
								  int len, int limit);
extern int	pg_mbcharcliplen(const char *mbstr, int len, int limit);
extern int	pg_database_encoding_max_length(void);
extern mbcharacter_incrementer pg_database_encoding_character_incrementer(void);

extern int	PrepareClientEncoding(int encoding);
