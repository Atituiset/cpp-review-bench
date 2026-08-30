# auto-redis-7f96cf2244

- 来源仓: redis/redis
- PR: #15628 (https://github.com/redis/redis/pull/15628)
- 命中工具: pr-mining
- scenario: cwe-476
- bug 锚点行: 3（原始 PR diff 行 489）
- judge: PR #15628 Fix potential defrag issues in the hash template registry :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）
