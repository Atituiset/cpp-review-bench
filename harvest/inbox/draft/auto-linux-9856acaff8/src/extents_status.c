// AUTO-DRAFT from torvalds/linux PR #1dbb7422053fa0f4fb5f9acc3931617fc0091939
}
  // <<< BUG ANCHOR
void ext4_es_unregister_shrinker(struct ext4_sb_info *sbi)
{
	percpu_counter_destroy(&sbi->s_es_stats.es_stats_cache_hits);
	percpu_counter_destroy(&sbi->s_es_stats.es_stats_cache_misses);
	percpu_counter_destroy(&sbi->s_es_stats.es_stats_all_cnt);
	percpu_counter_destroy(&sbi->s_es_stats.es_stats_shk_cnt);
	shrinker_free(sbi->s_es_shrinker);
}

/*
