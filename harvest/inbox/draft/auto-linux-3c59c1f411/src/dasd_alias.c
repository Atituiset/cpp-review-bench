// AUTO-DRAFT from torvalds/linux PR #e93fd809fb5692d1571bd008988ba734fd7fc976
	cqr->buildclk = get_tod_clock();
	cqr->status = DASD_CQR_FILLED;
  // <<< BUG ANCHOR
	rc = dasd_sleep_on_immediatly(cqr);
	return rc;
}
