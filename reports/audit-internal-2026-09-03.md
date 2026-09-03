# Golden 独立内审记录（2026-09-03）

> 性质：标注审计（建成线 §7.1 缺口）的第一轮——仓内独立复核，非第三方。
> 方法：两名独立复核者分别对 contract 轨 16 例、defect 轨 14 例逐例在代码本体上
> 重新推导 golden 判定（不轻信 notes/注释结论），must_find 需构造可达触发输入，
> must_not_find 需验证安全前提。触发契机：r01 被证实为「伪缺陷」（代数上不成立）。

## 结论汇总

| 轨 | 确认成立 | 场景标错（缺陷真、标签错） | 存疑 | 伪缺陷 |
|---|---|---|---|---|
| contract（16） | 10 | c01b、c03、t01 | c15、c21 | c12 |
| defect（14） | 6 | r02、r05、r12 | r08、r11、r13 | r07、r09 |

**系统性模式**：作者倾向把「守卫截断后的 off-by-one 越界」误述为「整数回绕
cwe-190」或标错读写方向（cwe-125/787 混淆）。r07/r09/c12 与旧 r01 同性质——
golden 声称的内存缺陷在代码本体上不可达，按现 golden 评分会把「正确不报」判成
FN、「瞎报」判成 TP，污染方向比场景标错更严重，应优先修复。

## 伪缺陷（缺陷代数上不存在，最优先）

### c12-intended-wrap-seq（contract）
`pos = (uint8_t)(seq + idx)` 本身就是 mod 256 约简，`pos ∈ [0,255]` 恒成立，
对 256 项 `ring` 不可能越界。「未约简」在类型层面为假。
修复方向：环形缓冲改小（如 64 项）使原序号直索引真实越界（同 r01 重写手法）。

### r07-alloc-size-wrap（defect）
清零循环上界与 malloc 尺寸是同一回绕表达式 `n * size`（uint32_t 域），
写入次数恒等于分配大小，cwe-787 越界写不可达（仅有无后续危害的 cwe-190）。
最小修复：清零循环改 `for (size_t i = 0; i < (size_t)n * size; i++)`。

### r09-double-free-errorpath（defect）
错误路径 `free(c->buf)` 后已 `c->buf = NULL`，末尾 `free(NULL)` 是 no-op，
cwe-415 不存在。最小修复：删掉错误路径里的 `c->buf = NULL;`。

## 场景标错（缺陷真实，scenario/机理错）

| 例 | 现标 | 实际 | 说明 |
|---|---|---|---|
| c01b | cwe-190 回绕 | cwe-787 off-by-one 越界写 | validate 已限 len≤128，回绕不可能；len≥127 时 sum≥128 越界写 payload[128] |
| c03 | cwe-190 回绕 | cwe-787 越界写 | MSG_REQUIRE(len≤64) 排除回绕；len≥63 时 buf[total] 越界 |
| t01 | cwe-125 越界读 | cwe-787 越界写 | handlers[3] 越界写，notes 内文自相矛盾 |
| r02 | cwe-125 | cwe-787 | rationale 自述「越界写」 |
| r05 | cwe-125 | cwe-787 | 同上标错模式 |
| r12 | cwe-190 + 「signed/unsigned 转换」机理 | 缺下界检查（cwe-129/125） | `LIM 16` 是有符号 int，不存在无符号转换；最小修复：`#define LIM 16u` 让 golden 机理成立 |

## 存疑（需设计级决策）

- **c15-spsc-lockfree-queue**：must_not_find 豁免 cwe-362 不成立——非原子共享
  head/tail 在 C11 下是真实 data race（无 _Atomic/内存序），工具报 cwe-362 属
  正确报告。修复方向：改为正确的 _Atomic + memory_order 实现（must_find 的
  g_tail 不取模缺陷保留），豁免才成立。
- **c21-startup-global-init**：must_find「边界检查失效」不成立（end 恒 <256、
  恒在界内），仅剩 off=255 覆盖 g_buf[0] 的逻辑瑕疵；且与 c12 判安全的模式
  同构、口径矛盾。修复方向：缓冲改小使回绕后越界真实发生，或降级改写 rationale。
- **r08-missing-lock-increment**：must_not_find「单线程读无竞争」前提与多线程
  前提不自洽。修复方向：在 notes/契约中明确「读仅在写线程 join 后调用」
  （join 建立 happens-before），豁免即成立。
- **r11-partial-stage-artifact**：must_not_find 锚点位于 STAGED_TREE=0 死分支，
  FP 探针无法被触发，载体无效。修复方向：在活动树中加一个受界安全访问器作
  must_not_find。
- **r13-state-missing-transition**：EV_A/EV_B 槽位同样显式置 0，与 EV_C 缺失
  运行时行为等价，正负例不可区分。修复方向：EV_A/EV_B 给真实 handler，仅
  EV_C 缺失。

## 附带发现（不影响现有 golden 判分，建议记录）

- c02 存在 golden 未列的真缺陷：c==NULL 时管线不停，stage_decrypt 空解引用。
- c01 的 contract.yaml a1 措辞与代码不符（guti_group_size 是第二条无判空路径，
  不影响豁免结论）。
- c02/c06 注释把缺陷行为描述成设计意图（去泄漏改写痕迹，注释-语义矛盾）。
- r03 的 must_find anchor `(void)o->v;` 在文件中匹配两处，靠 function 消歧。
- r14 与 r04 形态几乎重复（仅常数与参数名不同）。

## 确认成立清单

- contract（10）：c01、c02、c06、c08、c16、i01、i06、m01、m03、t02
- defect（6）：r01（重写后）、r03、r04、r06、r10、r14

## 后续

- [x] 按上表修复 golden/源码（2026-09-03 已完成：伪缺陷 3 例重写、场景标错 6 例纠正、存疑 5 例修复，全部 ASan/pthread 构造性验证）
- [x] 修复后重跑受影响基线（2026-09-03 已完成：`baseline-v3-2026-09-03.md`；另经主会话批准落地两项口径修订：cwe-125/787 家族合并、r11 场景 build→cwe-125）
- [ ] 第三方复核（建成线要求的正式标注审计）以本记录为输入
