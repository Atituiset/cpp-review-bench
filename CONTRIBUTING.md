# 贡献指南（CONTRIBUTING）

本仓用例是**真实 C/C++ 代码场景**基准，质量高于数量。新用例须过评审才能入库。

## 1. 五文件齐备

每个 `cases/<track>/<id>/` 必须包含：

| 文件 | 必填 | 说明 |
|---|---|---|
| `src/*.c[pp]` | 是 | 完整可编译源（per-case 不强制 CMake，但顶层 AllCases 需能进 compdb） |
| `CMakeLists.txt` | 否 | 独立可编译入口（设计允许，当前由顶层聚合） |
| `golden.json` | 是 | 标注契约 v2，过 `schema/golden.schema.json` |
| `contract.yaml` | contract 轨必填 | 契约声明（design §3 P4） |
| `notes.md` | 是 | 三段式：真实仓形态 / 为什么契约安全或真缺陷 / 各工具误判方式 |

## 2. 军规检查清单（提交前自审）

- [ ] `src/` 全部 `-Wall` 无 error，能进统一 compdb（本地 `cmake -S . -B build` 验证）
- [ ] `golden.json` 过 `tools/check_cases.py`（schema + id/轨一致 + anchor/file 真实存在）
- [ ] `golden` 的 `anchor` 在 `src` 中真实存在（脚本断言）
- [ ] 无「试验/播种」字样，注释为协议栈实战口吻
- [ ] contract 轨 `golden.context.contract` 与 `contract.yaml` 的 `contract.name` 一致

## 3. 评审流程（双人复核制）

1. **draft**：新用例提 PR，PR 描述填自评表（场景出处 / 契约可验证性 / 预期误报面 / 预期漏报面）。
2. **reviewed**：第二名复核者核对 golden 判定与 anchor 真实性，签字（PR approve）。
3. **active**：合并入 `main`，进入评分。
4. **retired**：契约形态废弃或设计演进时移出主集。

## 4. 标注审计

每类（contract/defect）抽 ≥3 例第三方复核 golden 判定，出审计记录（对齐 research.md 双人共识协议），登记于 `reports/`。

## 5. 提交信息规范

中文 conventional 格式：`feat:` / `fix:` / `docs:` / `ci:`。
