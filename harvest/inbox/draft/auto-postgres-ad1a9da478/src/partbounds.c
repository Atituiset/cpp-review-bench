// AUTO-DRAFT from postgres/postgres PR #9f191dce61ebf0ae77bf251b6383915859d7520b
  // <<< BUG ANCHOR
				if (partdesc->nparts > 0)
				{
					int			greatest_modulus;
					int			remainder;
					int			offset;

					/*
