// AUTO-DRAFT from torvalds/linux PR #b312ada03437d5874a8dbac6b4a1d479c2af3da9
			break;
		}

		status = a_ops->write_begin(iocb, mapping, pos, bytes,
						&folio, &fsdata);
		if (unlikely(status < 0))
