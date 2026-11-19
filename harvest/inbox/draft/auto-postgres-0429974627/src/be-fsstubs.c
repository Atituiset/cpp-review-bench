// AUTO-DRAFT from postgres/postgres PR #2c077c7d602dba034e209227ea04a14b10797c42
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

int
lo_read(int fd, char *buf, int len)
{
	int			status;
	LargeObjectDesc *lobj;

	if (fd < 0 || fd >= cookies_size || cookies[fd] == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("invalid large-object descriptor: %d", fd)));
	lobj = cookies[fd];

	/*
	 * Check state.  inv_read() would throw an error anyway, but we want the
	 * error to be about the FD's state not the underlying privilege; it might
	 * be that the privilege exists but user forgot to ask for read mode.
	 */
	if ((lobj->flags & IFS_RDLOCK) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("large object descriptor %d was not opened for reading",
						fd)));

	status = inv_read(lobj, buf, len);

	return status;
}
/* …（同文件无关代码省略）… */
	if (len < 0)
		len = 0;

	retval = (bytea *) palloc(VARHDRSZ + len);
	totalread = lo_read(fd, VARDATA(retval), len);
	SET_VARSIZE(retval, totalread + VARHDRSZ);
