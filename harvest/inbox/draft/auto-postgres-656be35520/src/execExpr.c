// AUTO-DRAFT from postgres/postgres PR #b6e41c16f8ccbfca0d51c94bdb6043b6e8393d6f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

	Datum	   *domainval = NULL;
	bool	   *domainnull = NULL;
	ListCell   *l;

	scratch->d.domaincheck.resulttype = ctest->resulttype;
	/* we'll allocate workspace only if needed */
/* …（同文件无关代码省略）… */

		scratch->d.domaincheck.constraintname = con->name;

		switch (con->constrainttype)
		{
			case DOM_CONSTRAINT_NOTNULL:
/* …（同文件无关代码省略）… */
				break;
		}
	}
}

/*
