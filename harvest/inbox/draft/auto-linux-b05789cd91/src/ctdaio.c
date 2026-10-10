// AUTO-DRAFT from torvalds/linux PR #5f6ee187fa3a7b3cee4b9dae6c3dda6d1d3237db
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

static int dao_clear_input(struct dao *dao, unsigned int start, unsigned int end)
{
	unsigned int i;

	if (!dao->imappers[start])
		return 0;
	for (i = start; i < end; i++) {
		dao->mgr->imap_delete(dao->mgr, dao->imappers[i]);
		dao->imappers[i] = NULL;
	}

	return 0;
}
