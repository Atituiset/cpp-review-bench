# auto-redis-99e2abe4b6

- 来源仓: redis/redis
- PR: #15594 (https://github.com/redis/redis/pull/15594)
- 命中工具: pr-mining
- scenario: cwe-476
- bug 锚点行: 3（原始 PR diff 行 628）
- judge: PR #15594 Fix use-after-free in handleClientsBlockedOnKey :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）
