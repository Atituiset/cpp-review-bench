// AUTO-DRAFT from postgres/postgres PR #4b6735c21fa4c43be041c6c610d7be6463c2c1f9
	PG_RETURN_FLOAT8((double) a2 - (double) a1);
}

/*
 * Compute the distance between two tid values (by mapping them to float8 and
 * then subtracting them).
