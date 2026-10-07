// AUTO-DRAFT from postgres/postgres PR #73d4ba85059bc08b9be8f65a3fd9d1ed13eb979b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
		Py_tp_dealloc, PLy_cursor_dealloc
	},
	{
		Py_tp_doc, (char *) PLy_cursor_doc
	},
	{
		Py_tp_iter, PyObject_SelfIter
/* …（同文件无关代码省略）… */
static void
PLy_cursor_dealloc(PLyCursorObject *self)
{
#if PY_VERSION_HEX >= 0x03080000
	PyTypeObject *tp = Py_TYPE(self);
#endif
	Portal		portal;

	if (!self->closed)
	{
		portal = GetPortalByName(self->portalname);

		if (PortalIsValid(portal))
		{
			UnpinPortal(portal);
			SPI_cursor_close(portal);
		}
		self->closed = true;
	}
	if (self->mcxt)
	{
		MemoryContextDelete(self->mcxt);
		self->mcxt = NULL;
	}

	PyObject_Free(self);
#if PY_VERSION_HEX >= 0x03080000
	/* This was not needed before Python 3.8 (Python issue 35810) */
	Py_DECREF(tp);
#endif
}
