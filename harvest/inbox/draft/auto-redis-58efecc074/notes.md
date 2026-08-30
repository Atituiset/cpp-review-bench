# auto-redis-58efecc074

- 来源仓: redis/redis
- PR: #15604 (https://github.com/redis/redis/pull/15604)
- 命中工具: pr-mining
- scenario: cwe-415
- bug 锚点行: 6（原始 PR diff 行 2058）
- judge: PR #15604 Fix hashTypeAllocSize() for TMPL_ARRAY :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）
