# auto-curl-0a10437ecd

- 来源仓: curl/curl
- PR: #15289 (https://github.com/curl/curl/pull/15289)
- 命中工具: pr-mining
- scenario: cwe-787
- judge: PR #15289 lib: fix function pointers to please UndefinedBehaviorSanitizer :: 启发式命中 \bmemcpy\s*\(|\bstrcpy\b|\bstrncpy\b|\bmemmove\b
