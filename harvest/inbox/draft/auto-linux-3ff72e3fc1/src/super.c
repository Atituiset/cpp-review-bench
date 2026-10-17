// AUTO-DRAFT from torvalds/linux PR #1dbb7422053fa0f4fb5f9acc3931617fc0091939
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	}
}

static void ext4_put_super(struct super_block *sb)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
/* …（同文件无关代码省略）… */
	ext4_stop_mmpd(sbi);

	brelse(sbi->s_sbh);
	sb->s_fs_info = NULL;
	/*
	 * Now that we are completely done shutting down the
	 * superblock, we need to actually destroy the kobject.
/* …（同文件无关代码省略）… */
#if IS_ENABLED(CONFIG_UNICODE)
	utf8_unload(sb->s_encoding);
#endif
	kfree(sbi);
}

static struct kmem_cache *ext4_inode_cachep;
/* …（同文件无关代码省略）… */
	/* Drain deferred EA inode iputs from journal replay */
	flush_delayed_work(&sbi->s_ea_inode_work);
	ext4_es_unregister_shrinker(sbi);
failed_mount3:
	/* flush s_sb_upd_work before sbi destroy */
	flush_work(&sbi->s_sb_upd_work);
