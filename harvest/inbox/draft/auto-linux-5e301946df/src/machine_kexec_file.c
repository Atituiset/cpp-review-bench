// AUTO-DRAFT from torvalds/linux PR #ac302aea3d824ccc99f29b8e499c365cb9a684a9
			vfree(headers);
			goto out_err;
		}
		image->elf_headers = headers;
		image->elf_load_addr = kbuf.mem;
		image->elf_headers_sz = headers_sz;
