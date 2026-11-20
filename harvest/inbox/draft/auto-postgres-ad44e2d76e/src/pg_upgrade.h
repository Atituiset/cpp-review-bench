// AUTO-DRAFT from postgres/postgres PR #32dd742b075e999fa358570bc13778b93f8025d4
typedef struct
{
	char	   *db_collate;
	char	   *db_ctype;
	char		db_collprovider;
	char	   *db_locale;
	int			db_encoding;
} DbLocaleInfo;
