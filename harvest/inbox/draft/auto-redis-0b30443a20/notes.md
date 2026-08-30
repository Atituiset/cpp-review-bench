# auto-redis-0b30443a20

- 来源仓: redis/redis
- PR: #15489 (https://github.com/redis/redis/pull/15489)
- 命中工具: pr-mining
- scenario: cwe-476
- bug 锚点行: 2（原始 PR diff 行 3813）
- judge: PR #15489 Fix unloadable RDB when XSETID lowers entries_added :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）
