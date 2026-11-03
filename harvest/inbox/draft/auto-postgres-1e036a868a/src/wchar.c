// AUTO-DRAFT from postgres/postgres PR #3ba886b35a2804dcd275868a19135a31ceb81386
#define NONUTF8_INVALID_BYTE1 (' ')


/*
 * Operations on multi-byte encodings are driven by a table of helper
 * functions.
 *
 * To add an encoding support, define mblen(), dsplen(), verifychar() and
 * verifystr() for the encoding.  For server-encodings, also define mb2wchar()
 * and wchar2mb() conversion functions.
 *
 * These functions generally assume that their input is validly formed.
 * The "verifier" functions, further down in the file, have to be more
 * paranoid.
 *
 * We expect that mblen() does not need to examine more than the first byte
 * of the character to discover the correct length.  GB18030 is an exception
 * to that rule, though, as it also looks at second byte.  But even that
 * behaves in a predictable way, if you only pass the first byte: it will
 * treat 4-byte encoded characters as two 2-byte encoded characters, which is
 * good enough for all current uses.
 *
 * Note: for the display output of psql to work properly, the return values
 * of the dsplen functions must conform to the Unicode standard. In particular
 * the NUL character is zero width and control characters are generally
 * width -1. It is recommended that non-ASCII encodings refer their ASCII
 * subset to the ASCII routines to ensure consistency.
 */

/* No error-reporting facility.  Ignore incomplete trailing byte sequence. */
#define MB2CHAR_NEED_AT_LEAST(len, need) if ((len) < (need)) break

/* …（同文件无关代码省略）… */

/*
 * Convert pg_wchar to EUC_* encoding.
 * caller must allocate enough space for "to", including a trailing zero!
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_wchar2euc_with_len(const pg_wchar *from, unsigned char *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		unsigned char c;

		if ((c = (*from >> 24)))
		{
			*to++ = c;
			*to++ = (*from >> 16) & 0xff;
			*to++ = (*from >> 8) & 0xff;
			*to++ = *from & 0xff;
			cnt += 4;
		}
		else if ((c = (*from >> 16)))
		{
			*to++ = c;
			*to++ = (*from >> 8) & 0xff;
			*to++ = *from & 0xff;
			cnt += 3;
		}
		else if ((c = (*from >> 8)))
		{
			*to++ = c;
			*to++ = *from & 0xff;
			cnt += 2;
		}
		else
		{
			*to++ = *from;
			cnt++;
		}
		from++;
		len--;
	}
	*to = 0;
	return cnt;
}
/* …（同文件无关代码省略）… */

/*
 * convert UTF8 string to pg_wchar (UCS-4)
 * caller must allocate enough space for "to", including a trailing zero!
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_utf2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;
	uint32		c1,
				c2,
				c3,
				c4;

	while (len > 0 && *from)
	{
		if ((*from & 0x80) == 0)
		{
			*to = *from++;
			len--;
		}
		else if ((*from & 0xe0) == 0xc0)
		{
			MB2CHAR_NEED_AT_LEAST(len, 2);
			c1 = *from++ & 0x1f;
			c2 = *from++ & 0x3f;
			*to = (c1 << 6) | c2;
			len -= 2;
		}
		else if ((*from & 0xf0) == 0xe0)
		{
			MB2CHAR_NEED_AT_LEAST(len, 3);
			c1 = *from++ & 0x0f;
			c2 = *from++ & 0x3f;
			c3 = *from++ & 0x3f;
			*to = (c1 << 12) | (c2 << 6) | c3;
			len -= 3;
		}
		else if ((*from & 0xf8) == 0xf0)
		{
			MB2CHAR_NEED_AT_LEAST(len, 4);
			c1 = *from++ & 0x07;
			c2 = *from++ & 0x3f;
			c3 = *from++ & 0x3f;
			c4 = *from++ & 0x3f;
			*to = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;
			len -= 4;
		}
		else
		{
			/* treat a bogus char as length 1; not ours to raise error */
			*to = *from++;
			len--;
		}
		to++;
		cnt++;
	}
	*to = 0;
	return cnt;
}
/* …（同文件无关代码省略）… */

/*
 * Trivial conversion from pg_wchar to UTF-8.
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_wchar2utf_with_len(const pg_wchar *from, unsigned char *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		int			char_len;

		unicode_to_utf8(*from, to);
		char_len = pg_utf_mblen(to);
		cnt += char_len;
		to += char_len;
		from++;
		len--;
	}
	*to = 0;
	return cnt;
}
/* …（同文件无关代码省略）… */
int
pg_utf_mblen(const unsigned char *s)
{
	int			len;

	if ((*s & 0x80) == 0)
		len = 1;
	else if ((*s & 0xe0) == 0xc0)
		len = 2;
	else if ((*s & 0xf0) == 0xe0)
		len = 3;
	else if ((*s & 0xf8) == 0xf0)
		len = 4;
#ifdef NOT_USED
	else if ((*s & 0xfc) == 0xf8)
		len = 5;
	else if ((*s & 0xfe) == 0xfc)
		len = 6;
#endif
	else
		len = 1;
	return len;
}
/* …（同文件无关代码省略）… */
/*
 * Trivial conversion from pg_wchar to single byte encoding. Just ignores
 * high bits.
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_wchar2single_with_len(const pg_wchar *from, unsigned char *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		*to++ = *from++;
		len--;
		cnt++;
	}
	*to = 0;
	return cnt;
}
/* …（同文件无关代码省略）… */
/*
 *-------------------------------------------------------------------
 * multibyte sequence validators
 *
 * The verifychar functions accept "s", a pointer to the first byte of a
 * string, and "len", the remaining length of the string.  If there is a
 * validly encoded character beginning at *s, return its length in bytes;
 * else return -1.
 *
 * The verifystr functions also accept "s", a pointer to a string and "len",
 * the length of the string.  They verify the whole string, and return the
 * number of input bytes (<= len) that are valid.  In other words, if the
 * whole string is valid, verifystr returns "len", otherwise it returns the
 * byte offset of the first invalid character.  The verifystr functions must
 * test for and reject zeroes in the input.
 *
 * The verifychar functions can assume that len > 0 and that *s != '\0', but
 * they must test for and reject zeroes in any additional bytes of a
 * multibyte character.  Note that this definition allows the function for a
 * single-byte encoding to be just "return 1".
 *-------------------------------------------------------------------
 */
static int
pg_ascii_verifychar(const unsigned char *s, int len)
{
	return 1;
}
/* …（同文件无关代码省略）… */

/*
 * Verify the first multibyte character of the g
