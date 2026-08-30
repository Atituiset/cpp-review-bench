# auto-redis-c920dd0968

- 来源仓: redis/redis
- PR: #15433 (https://github.com/redis/redis/pull/15433)
- 命中工具: pr-mining
- scenario: cwe-787
- bug 锚点行: 3（原始 PR diff 行 733）
- judge: PR #15433 Fix signed overflow in BITFIELD #offset parsing :: PR 修复动作推断：修复前越界访问（加边界检查）
