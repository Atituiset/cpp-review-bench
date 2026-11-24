// AUTO-DRAFT from postgres/postgres PR #9ebc9a458c38465cd9609814cfc9ca3a2f46fef5
  // <<< BUG ANCHOR
				if (!crc_ok)
					ereport(ERROR,
							(errmsg("calculated CRC checksum does not match value stored in file")));

				/* Send the full file, zero-padded as it is on disk */
				controlFile = palloc0(PG_CONTROL_FILE_SIZE);
