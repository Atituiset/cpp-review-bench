# auto-redis-071866b6f1

- 来源仓: redis/redis
- PR: #15462 (https://github.com/redis/redis/pull/15462)
- 命中工具: pr-mining
- scenario: cwe-476
- bug 锚点行: 4（原始 PR diff 行 1087）
- judge: PR #15462 Fix dictMemUsage() over-counting no_value entries :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作
