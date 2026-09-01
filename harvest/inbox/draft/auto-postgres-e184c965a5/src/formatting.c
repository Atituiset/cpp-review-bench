// AUTO-DRAFT from postgres/postgres PR #3b120b1e94dd0387ca40e1e356f6b7eb8793d297
#define NUM_F_MULTI			(1 << 11)
/* …（同文件无关代码省略）… */
#define IS_MULTI(_f)	((_f)->flag & NUM_F_MULTI)
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
		if (IS_MULTI(&Num))
		{
			orgnum = DatumGetCString(DirectFunctionCall1(int4out,
														 Int32GetDatum(value * ((int32) pow((double) 10, (double) Num.multi)))));
			Num.pre += Num.multi;
		}
		else
		{
			orgnum = DatumGetCString(DirectFunctionCall1(int4out,
														 Int32GetDatum(value)));
		}

		if (*orgnum == '-')
		{
