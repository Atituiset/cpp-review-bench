// AUTO-DRAFT from postgres/postgres PR #a625e153b7d2f0e1c81955d13f2fc8de472bc082
typedef uint16 WordEntryPos;
/* …（同文件无关代码省略）… */
typedef struct
{
	uint16		npos;
	WordEntryPos pos[1];  // <<< BUG ANCHOR
} WordEntryPosVector1;

#define MAXNUMPOS	(256)		/* semi-arbitrary limit on npos */
