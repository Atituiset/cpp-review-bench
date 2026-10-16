// AUTO-DRAFT from postgres/postgres PR #3c157ad116715aba84d3fb828a91aeaad002f92a
			if (chunkblock != block)
				elog(WARNING, "problem in Bump %s: bogus block link in block %p, chunk %p",
					 name, block, chunk);
		}

		if (has_external_chunk && nchunks > 1)
