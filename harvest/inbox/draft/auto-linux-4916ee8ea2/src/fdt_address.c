// AUTO-DRAFT from torvalds/linux PR #21a7f44fd0765fcc5194130b28767786eba1d1e7
		goto bail;
	bus = &of_busses[0];

	/* Cound address cells & copy address locally */
	bus->count_cells(blob, parent, &na, &ns);
	if (!OF_CHECK_COUNTS(na, ns)) {
		pr_err("Bad cell count for %s\n",
