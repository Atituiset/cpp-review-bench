// AUTO-DRAFT from postgres/postgres PR #45fc51fcbf4534ea2f6f85e36f8155dbd5ba9851
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	/* If it's a row header, fix row member flags and strategies similarly */
	if (skey->sk_flags & SK_ROW_HEADER)
	{
		ScanKey		subkey = (ScanKey) DatumGetPointer(skey->sk_argument);

		if (subkey->sk_flags & SK_ISNULL)
		{
			/* First row member is NULL, so RowCompare is unsatisfiable */
			Assert(subkey->sk_flags & SK_ROW_MEMBER);
			return false;
		}

		for (;;)
		{
/* …（同文件无关代码省略）… */
				break;
			subkey++;
		}
	}

	return true;
