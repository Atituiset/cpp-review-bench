// AUTO-DRAFT from torvalds/linux PR #b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb
	/* Gracefully handle the B0 case: fall back to B9600 */
	fuart = fuart ? fuart : 9600 * 16;

	/* Get Fuart closer to Fref */
	fuart *= rounddown_pow_of_two(fref / fuart);
