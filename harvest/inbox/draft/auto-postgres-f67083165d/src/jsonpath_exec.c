// AUTO-DRAFT from postgres/postgres PR #a625e153b7d2f0e1c81955d13f2fc8de472bc082
#define BASE_JVL_ITEMS 2		/* number of items a base chunk holds */
/* …（同文件无关代码省略）… */
typedef struct JsonValueList
{
	int			nitems;			/* number of items stored in this chunk */
	int			maxitems;		/* allocated length of items[] */
	struct JsonValueList *next; /* => next chunk, if any */
	struct JsonValueList *last; /* => last chunk (only valid in base chunk) */
	JsonbValue	items[BASE_JVL_ITEMS];  // <<< BUG ANCHOR
} JsonValueList;

/* State data for iterating through a JsonValueList */
