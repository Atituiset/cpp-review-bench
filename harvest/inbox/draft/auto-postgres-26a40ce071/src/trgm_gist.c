// AUTO-DRAFT from postgres/postgres PR #8aea84d2b8a1233ae44bb0dc0fde2a2ab0f5e35f
#define GETENTRY(vec,pos) ((TRGM *) DatumGetPointer((vec)->vector[(pos)].key))
/* …（同文件无关代码省略）… */
		LOOPBYTE(siglen)
		{
			if ((sign[i] & 0xff) != 0xff)
				PG_RETURN_POINTER(retval);
		}
/* …（同文件无关代码省略）… */
static int32
unionkey(BITVECP sbase, TRGM *add, int siglen)
{
	if (ISSIGNKEY(add))
	{
		int32		i;
		BITVECP		sadd = GETSIGN(add);
  // <<< BUG ANCHOR
		if (ISALLTRUE(add))
			return 1;

		LOOPBYTE(siglen)
			sbase[i] |= sadd[i];
	}
	else
	{
		trgm	   *ptr = GETARR(add);
		int32		tmp = 0;

		for (unsigned i = 0; i < ARRNELEM(add); i++)
		{
			CPTRGM(&tmp, ptr + i);
			HASH(sbase, tmp, siglen);
		}
	}
	return 0;
}
/* …（同文件无关代码省略）… */
	{
		if (unionkey(base, GETENTRY(entryvec, i), siglen))
		{
			result->flag = ALLISTRUE;
			SET_VARSIZE(result, CALCGTSIZE(ALLISTRUE, siglen));
			break;
		}
