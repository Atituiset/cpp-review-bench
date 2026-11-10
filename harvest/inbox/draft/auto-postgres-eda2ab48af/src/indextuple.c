// AUTO-DRAFT from postgres/postgres PR #245eb832f88631c986a2dadad2cbe94f57089fd6
	Assert(IndexTupleSize(truncated) <= IndexTupleSize(source));
  // <<< BUG ANCHOR
	/*
	 * Cannot leak memory here, TupleDescCopy() doesn't allocate any inner
	 * structure, so, plain pfree() should clean all allocated memory
	 */
	pfree(truncdesc);
