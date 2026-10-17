// AUTO-DRAFT from postgres/postgres PR #2323f03a1789bc52bee980c56b8057470458701a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR

	if (nextMXact == tmpMXact)
	{
		/* Corner case 1: there is no next multixact */
		nextMXOffset = nextOffset;
	}
	else
/* …（同文件无关代码省略）… */
		offptr = (MultiXactOffset32 *) buf;
		offptr += entryno;
		nextMXOffset = *offptr;
	}

	if (nextMXOffset == 0)
	{
		/* Invalid entry.  These can be left behind on a server crash. */
		return false;
	}
	length = nextMXOffset - offset;

	if (length < 0)
