// AUTO-DRAFT from torvalds/linux PR #c58471d848a7a62c0651d85983743e9e6b2e6396
			vfree(headers);
			goto out_err;
		}
		image->elf_headers = headers;
		image->elf_load_addr = kbuf.mem;
		image->elf_headers_sz = headers_sz;
