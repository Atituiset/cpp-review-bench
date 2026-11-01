# auto-linux-7a0724601c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0](https://github.com/torvalds/linux/commit/e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0 (https://github.com/torvalds/linux/commit/e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0 Merge tag 'sound-fix-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/tiwai/sound :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -117,6 +117,7 @@ struct snd_info_entry *snd_info_create_module_entry(struct module *module,
 struct snd_info_entry *snd_info_create_card_entry(struct snd_card *card,
 					     const char *name,
 					     struct snd_info_entry *parent);
+void snd_info_disconnect_entry(struct snd_info_entry *entry);
 void snd_info_free_entry(struct snd_info_entry *entry);
 
 int snd_info_card_create(struct snd_card *card);
@@ -164,6 +165,7 @@ static inline int snd_info_get_line(struct snd_info_buffer *buffer, char *line,
 static inline char *snd_info_get_str(char *dest, char *src, int len) { return NULL; }
 static inline struct snd_info_entry *snd_info_create_module_entry(struct module *module, const char *name, struct snd_info_entry *parent) { return NULL; }
 static inline struct snd_info_entry *snd_info_create_card_entry(struct snd_card *card, const char *name, struct snd_info_entry *parent) { return NULL; }
+static inline void snd_info_disconnect_entry(struct snd_info_entry *entry) { }
 static inline void snd_info_free_entry(struct snd_info_entry *entry) { ; }
 
 static inline int snd_info_card_create(struct snd_card *card) { return 0; }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__poll_t`
- 外部函数：`int`
- 外部函数：`loff_t`
- 外部函数：`ssize_t`
- 外部函数：`void`
- 大写宏：`NULL`
- 外部类型：`file`
- 外部类型：`inode`
- 外部类型：`list_head`
- 外部类型：`loff_t`
- 外部类型：`module`
- 外部类型：`mutex`
- 外部类型：`proc_dir_entry`
- 外部类型：`size_t`
- 外部类型：`snd_card`
- 外部类型：`umode_t`
- 外部类型：`vm_area_struct`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-7a0724601c` → 本草稿移入 `cases/defect/auto-linux-7a0724601c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
