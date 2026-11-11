// AUTO-DRAFT from postgres/postgres PR #bc192c898b21e7479d51f7537e8f407672dfbe20

			/*
			 * Unlike fileIterateForeignScan(), we intentionally ignore
			 * reject_limit so that ANALYZE can collect statistics from
			 * valid rows even if the limit is exceeded.
			 */
			continue;
		}
