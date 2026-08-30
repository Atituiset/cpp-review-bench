# auto-redis-288f87f9e3

- 来源仓: redis/redis
- PR: #15530 (https://github.com/redis/redis/pull/15530)
- 命中工具: pr-mining
- scenario: cwe-476
- bug 锚点行: None（原始 PR diff 行 None）
- judge: PR #15530 Fix replica left as a sub-replica after a failover :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）
