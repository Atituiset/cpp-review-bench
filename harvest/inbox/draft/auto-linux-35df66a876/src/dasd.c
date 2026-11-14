// AUTO-DRAFT from torvalds/linux PR #e93fd809fb5692d1571bd008988ba734fd7fc976
		cqr->status = DASD_CQR_ERROR;
		return -EIO;
	}
	cqr->startclk = get_tod_clock();
	cqr->starttime = jiffies;
	cqr->retries--;
/* …（同文件无关代码省略）… */
	return rc;
}
  // <<< BUG ANCHOR
int dasd_sleep_on_immediatly(struct dasd_ccw_req *cqr)
{
	struct dasd_device *device;
	int rc;
/* …（同文件无关代码省略）… */

	return rc;
}
EXPORT_SYMBOL(dasd_sleep_on_immediatly);

/*
 * Cancels a request that was started with dasd_sleep_on_req.
