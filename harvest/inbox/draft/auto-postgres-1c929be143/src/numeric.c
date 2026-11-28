// AUTO-DRAFT from postgres/postgres PR #cf9d92d1f7550063aefbb219e1160e58ddb760ac
#define NUMERIC_PINF			0xD000
/* …（同文件无关代码省略）… */
#define NUMERIC_INF_SIGN_MASK	0x2000
/* …（同文件无关代码省略）… */
#define NUMERIC_IS_INF(n) \
	(((n)->choice.n_header & ~NUMERIC_INF_SIGN_MASK) == NUMERIC_PINF)
/* …（同文件无关代码省略）… */
	return NUMERIC_IS_INF(num);
}

/*
 * numeric_is_integral() -
 *
