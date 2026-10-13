// AUTO-DRAFT from postgres/postgres PR #cde33148c9d8cc432296992eaa5c2f58a126712d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
			tbinfo->typstorage[j] = *(PQgetvalue(res, r, i_typstorage));
			tbinfo->attidentity[j] = *(PQgetvalue(res, r, i_attidentity));
			tbinfo->attgenerated[j] = *(PQgetvalue(res, r, i_attgenerated));
			tbinfo->needs_override = tbinfo->needs_override || (tbinfo->attidentity[j] == ATTRIBUTE_IDENTITY_ALWAYS);
			tbinfo->attisdropped[j] = (PQgetvalue(res, r, i_attisdropped)[0] == 't');
			tbinfo->attlen[j] = atoi(PQgetvalue(res, r, i_attlen));
			tbinfo->attalign[j] = *(PQgetvalue(res, r, i_attalign));
			tbinfo->attislocal[j] = (PQgetvalue(res, r, i_attislocal)[0] == 't');
