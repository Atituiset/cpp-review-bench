# consumers/ —— 工具团队消费入口

cpp-review-bench 对一切消费方只暴露一个契约：**归一化 findings（`schema/findings.schema.json`）→ `tools/eval.py` 评分**。
你不需要理解双轨结构、golden 格式或评分细节——把自家工具的结果转成归一化 findings，剩下的由本仓完成。

## 三种消费路径

| 路径 | 适合谁 | 入口 |
|---|---|---|
| **本地直跑** | 想在开发机上快速试分的工具团队 | `consumers/local/run.sh` |
| **reusable workflow** | 想把跑分挂进自己 CI 的工具团队 | `consumers/github-action/bench.yml`（workflow_call） |
| **只拿资产自己玩** | 索引/提取类工具、Agent+LLM 评审、定制评测流程 | 统一 compdb（`build/compile_commands.json`）或 `cases/<track>/<id>/src/` 源码树 |

### 1. 本地直跑（consumers/local/run.sh）

```bash
# (a) 你的分析器能直接产出归一化 findings（每例一个 <case_id>.json 放一个目录）：
consumers/local/run.sh --findings-dir <你的 findings 目录>

# (b) 你的分析器只认 compile_commands.json：
consumers/local/run.sh -- <你的分析器包装命令>
#   脚本幂等备好 build/compile_commands.json（已生成则跳过 cmake），
#   导出 $COMPDB / $CASES_DIR / $FINDINGS_DIR / $BENCH_ROOT 后执行你的命令；
#   包装命令把归一化 findings 写进 $FINDINGS_DIR，脚本随即自动评分。

# 只想要编译数据库：
consumers/local/run.sh --compdb-only
```

本仓 9 个已接入工具的完整本地链路（扫描 → 归一化 → 评分）见 `sa/runners/run_*.sh`，可直接抄包装写法。

### 2. reusable workflow（consumers/github-action/bench.yml）

你的仓 `.github/workflows/bench.yml` 三行接入：

```yaml
jobs:
  bench:
    uses: <org>/cpp-review-bench/.github/workflows/bench.yml@v1   # 占位 <org> 见下
    with:
      tool_command: '<tool> analyze --compdb "$COMPDB" --out "$FINDINGS_DIR"'
```

- **发布位置说明**：GitHub 规定被 `uses:` 引用的 reusable workflow 必须物理位于
  被引用仓的 `.github/workflows/` 下。本文件是源文件与权威版本；本仓发布时会把它
  镜像到 `.github/workflows/bench.yml`，外部按上面的 `uses:` 路径引用即可
  （`<org>` 替换为实际组织名）。在那之前，你也可以把本文件整份复制到自己仓的
  `.github/workflows/`，用 `uses: ./.github/workflows/bench.yml` 本地引用。
- **可用输入**：`tool_command`（必填）、`tool_image`（分析器在容器里跑）、
  `sarif_file`（SARIF 系工具免写 adapter，走 `sa/adapters/sarif_to_findings.py`）、
  `tool_name`、`bench_repo`、`bench_ref`（checkout 本仓的 ref；默认 `main` 适合冒烟，
  正式跑分请钉 tag/commit 并在报告中注明，可复现性是硬要求）。
- **fail-open 语义**：评测零发现（全 FN）不红——分数是数据不是门禁；
  脚手架失败（checkout / cmake / 自检门禁 / findings 不合 schema）要红。

### 3. 只拿资产自己玩

- **统一 compdb**：`cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` 一键生成
  `build/compile_commands.json`（clangd / navmap / CSA / 任何吃编译数据库的工具）。
  golden 的 `context.navmap_expect` 字段提供提取结果的判定锚点。
- **用例源码树**：`cases/contract/<id>/src/`（16 例）、`cases/defect/<id>/src/`（14 例），
  每例独立可编译，可直接指给分析器或物化为 diff/PR 喂 Agent+LLM 评审。
- 契约数据随车在 `cases/contract/<id>/contract.yaml`；golden 在每例 `golden.json`。

## findings 契约要点

格式权威定义：`schema/findings.schema.json`（冻结项，不可改）。要点：

- **顶层字段**：`tool`（工具名）、`track`（`contract`/`defect`/`calibration`）、
  `case_id`（对应 `cases/<track>/<case_id>`）、`version`（工具版本，钉死可复现）、
  `findings`（数组，可为空）。每例一个文件：`<case_id>.json`。
- **单条 finding**：必填 `file`（相对用例根，如 `src/guti.c`）+ `anchor`
  （被报语句的原文片段，L1 匹配主键，去空白子串比对）；可选 `scenario`
  （`cwe-<ID>`，可组合 `cwe-190+cwe-787`，可为 `null`）、`severity`
  （`critical`/`important`/`minor`）、`function`、`line`（anchor 缺失时走 ±tolerance
  兜底）、`message`、`verified`（经编译/复现验证，单列奖励）。`additionalProperties: false`，
  多余字段会被门禁拦下。
- **schema 校验门禁**：`tools/eval.py run` 默认对每个 findings 文件过 schema 校验，
  不合规直接失败（`--no-validate` 仅供调试兜底）。CI 侧另有 `tools/check_evidence.py`
  守住 `reports/evidence/` 归档。
- **eval 四态口径**（per case，详见 `docs/design-v0.4.md` §4）：
  `PASS`（must_find 全中且无违反）/ `FN`（漏报 must_find）/ `FP`（报中 must_not_find；
  contract 轨注入契约后仍报记**契约违反**，权重 > 裸 FP）/ `EXTRA`（golden 未吸收的多余 finding）。
  汇总给出 per-track pass 率、recall、severity 分级正确率、裸 FP vs 契约违反分列、verified 计数。

## 最小接入示例

以 cppcheck 为例（仓内现成链路，本地一条命令跑完全部 30 例并评分）：

```bash
sudo apt-get install -y cppcheck   # 或你的平台等价安装
sa/runners/run_cppcheck.sh /tmp/cppcheck-findings
# 等价于：逐例扫描 → cppcheck_to_findings.py 归一化 → tools/eval.py run 评分
```

假想工具 `<org>/<tool>`（SARIF 输出型）经 reusable workflow 接入：

```yaml
jobs:
  bench:
    uses: <org>/cpp-review-bench/.github/workflows/bench.yml@v1
    with:
      tool_image: 'ghcr.io/<org>/<tool>:1.2.3'
      tool_command: '<tool> scan --project "$BENCH_ROOT" --sarif > /workspace/out.sarif'
      sarif_file: out.sarif        # 免写 adapter，sarif_to_findings.py 统一归一化
      tool_name: '<tool>'
      bench_ref: v1.0.0            # 正式跑分钉 tag，别用 main
```

跑完在 Actions artifact `bench-report-<tool>` 里拿到 `bench-report.json`（四态汇总 + per-case 明细）。
欢迎把结果整理为 `reports/baseline-<tool>.md` 提 PR 登记（格式见 `reports/README.md`，数字标注「自测口径 + 工具版本」）。
