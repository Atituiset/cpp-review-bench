// AUTO-DRAFT from postgres/postgres PR #3ba886b35a2804dcd275868a19135a31ceb81386
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
pg_encoding_mbcliplen(int encoding, const char *mbstr,
					  int len, int limit)
{
	mblen_converter mblen_fn;
	int			clen = 0;
	int			l;

/* …（同文件无关代码省略）… */
int
GetDatabaseEncoding(void)
{
	return DatabaseEncoding->encoding;
}
/* …（同文件无关代码省略）… */
pg_generic_charinc(unsigned char *charptr, int len)
{
	unsigned char *lastbyte = charptr + len - 1;
	mbchar_verifier mbverify;

	/* We can just invoke the character verifier directly. */
	mbverify = pg_wchar_table[GetDatabaseEncoding()].mbverifychar;

	while (*lastbyte < (unsigned char) 255)
	{
		(*lastbyte)++;
		if ((*mbverify) (charptr, len) == len)
			return true;
	}

/* …（同文件无关代码省略）… */
int
pg_verify_mbstr_len(int encoding, const char *mbstr, int len, bool noError)
{
	mbchar_verifier mbverifychar;
	int			mb_len;

	Assert(PG_VALID_ENCODING(encoding));
