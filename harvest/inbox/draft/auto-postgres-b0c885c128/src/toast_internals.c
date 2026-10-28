// AUTO-DRAFT from postgres/postgres PR #27417270ab4e929aa3731e55945872206c425f75
#include "catalog/catalog.h"
#include "miscadmin.h"
#include "utils/fmgroids.h"
#include "utils/rel.h"
#include "utils/snapmgr.h"

/* …（同文件无关代码省略）… */
									&toastidxs,
									&num_indexes);

	/*
	 * Get the data pointer and length, and compute va_rawsize and va_extinfo.
	 *
