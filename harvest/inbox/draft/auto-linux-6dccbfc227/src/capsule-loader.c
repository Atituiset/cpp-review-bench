// AUTO-DRAFT from torvalds/linux PR #89d2d7bf0698b7fdfc914bfd219e739e9308d332
#define NO_FURTHER_WRITE_ACTION -1
/* …（同文件无关代码省略）… */
static void efi_free_all_buff_pages(struct capsule_info *cap_info)
{
	while (cap_info->index > 0)
		__free_page(cap_info->pages[--cap_info->index]);
  // <<< BUG ANCHOR
	cap_info->index = NO_FURTHER_WRITE_ACTION;
}
/* …（同文件无关代码省略）… */
		page = cap_info->pages[cap_info->index - 1];
	}

	kbuff = kmap(page);
	kbuff += PAGE_SIZE - cap_info->page_bytes_remain;

	/* Copy capsule binary data from user space to kernel space buffer */
/* …（同文件无关代码省略）… */
	}

	cap_info->count += write_byte;
	kunmap(page);

	/* Submit the full binary to efi_capsule_update() API */
	if (cap_info->header.headersize > 0 &&
/* …（同文件无关代码省略）… */
	return write_byte;

fail_unmap:
	kunmap(page);
failed:
	efi_free_all_buff_pages(cap_info);
	return ret;
