#!/usr/bin/env bash
# cpp-review-bench 本地消费入口（工具团队自助取用）。
#
# 两条接入路径（统一契约：归一化 findings（schema/findings.schema.json）→ tools/eval.py 评分）：
#
#   (a) 分析器能直接产出归一化 findings JSON：
#       把每例一个 <case_id>.json 放到某个目录，然后：
#         consumers/local/run.sh --findings-dir <目录>
#       脚本直接调 python3 tools/eval.py run <目录> 评分（默认过 schema 校验门禁）。
#
#   (b) 分析器只认 compile_commands.json：
#         consumers/local/run.sh -- <你的分析器包装命令>
#       脚本先确保统一 compdb 存在（cmake 配置幂等：已生成则跳过），
#       再导出以下环境变量并执行你的命令：
#         BENCH_ROOT   本仓根目录
#         COMPDB       build/compile_commands.json 的绝对路径
#         CASES_DIR    cases/ 目录（用例源码树）
#         FINDINGS_DIR 归一化 findings 落盘目录（默认 build/findings，--out-dir 可改；
#                      你的包装命令把每例一个 <case_id>.json 写到这里，脚本随即自动评分）
#
# 只想要 compdb、不接评分：
#   consumers/local/run.sh --compdb-only
#
# 用法：
#   consumers/local/run.sh --findings-dir <目录> [--no-contract]
#   consumers/local/run.sh [--out-dir <目录>] [--rebuild] -- <命令...>
#   consumers/local/run.sh --compdb-only [--rebuild]
#   consumers/local/run.sh -h | --help
#
# 选项：
#   --findings-dir <目录>  路径 (a)：直接评分已有归一化 findings 目录
#   --out-dir <目录>       路径 (b)：findings 落盘目录（默认 build/findings，build/ 已被 gitignore）
#   --compdb-only          只生成 compile_commands.json，不执行分析器、不评分
#   --rebuild              强制重新 cmake 配置（默认幂等：compdb 已存在则跳过）
#   --no-contract          透传给 eval.py：contract 轨按「未注入契约」口径评分
#   -h, --help             显示本说明
#
# 退出码语义：分析器/构建/自检失败非零即红；工具零发现（全 FN）不算失败——
# 评测分数是数据不是门禁（与本仓 CI 的 fail-open 口径一致）。
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT/build"
COMPDB="$BUILD_DIR/compile_commands.json"

FINDINGS_IN=""
OUT_DIR=""
COMPDB_ONLY=0
REBUILD=0
NO_CONTRACT=0
CMD=()

usage() { sed -n '2,39p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

while [ $# -gt 0 ]; do
  case "$1" in
    --findings-dir) FINDINGS_IN="${2:?--findings-dir 需要参数}"; shift 2 ;;
    --out-dir)      OUT_DIR="${2:?--out-dir 需要参数}"; shift 2 ;;
    --compdb-only)  COMPDB_ONLY=1; shift ;;
    --rebuild)      REBUILD=1; shift ;;
    --no-contract)  NO_CONTRACT=1; shift ;;
    -h|--help)      usage; exit 0 ;;
    --)             shift; CMD=("$@"); break ;;
    *)              echo "[FAIL] 未知参数: $1（--help 查看用法）" >&2; exit 2 ;;
  esac
done

EVAL_ARGS=()
[ "$NO_CONTRACT" -eq 1 ] && EVAL_ARGS+=(--no-contract)

# 确保统一 compdb 存在（幂等：已配置则跳过，--rebuild 强制重来）
ensure_compdb() {
  if [ "$REBUILD" -eq 0 ] && [ -f "$COMPDB" ]; then
    echo "[skip] compdb 已存在（--rebuild 可强制重建）: $COMPDB"
    return
  fi
  echo "[build] cmake 配置全量用例 + 导出 compile_commands.json ..."
  cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  [ -f "$COMPDB" ] || { echo "[FAIL] compdb 未生成: $COMPDB" >&2; exit 1; }
  echo "[ok] compdb: $COMPDB"
}

# 路径 (a)：直接评分已有 findings 目录
if [ -n "$FINDINGS_IN" ]; then
  [ -d "$FINDINGS_IN" ] || { echo "[FAIL] findings 目录不存在: $FINDINGS_IN" >&2; exit 1; }
  echo "=== 评测（路径 a：已有归一化 findings）==="
  exec python3 "$ROOT/tools/eval.py" run "$FINDINGS_IN" "${EVAL_ARGS[@]}"
fi

# 路径 (b)：先备 compdb
ensure_compdb

if [ "$COMPDB_ONLY" -eq 1 ]; then
  echo "compdb 就绪。把它指给你的分析器：$COMPDB"
  exit 0
fi

[ "${#CMD[@]}" -gt 0 ] || { echo "[FAIL] 缺少分析器命令（-- <命令...>，或 --help 查看用法）" >&2; exit 2; }

FINDINGS_DIR="${OUT_DIR:-$BUILD_DIR/findings}"
mkdir -p "$FINDINGS_DIR"

export BENCH_ROOT="$ROOT" COMPDB="$COMPDB" CASES_DIR="$ROOT/cases" FINDINGS_DIR="$FINDINGS_DIR"
echo "=== 执行分析器包装命令 ==="
echo "    COMPDB=$COMPDB"
echo "    FINDINGS_DIR=$FINDINGS_DIR"
"${CMD[@]}"

# 命令成功后：findings 目录有产物则评分，没有则提示（零产物不视为失败）
shopt -s nullglob
jsons=("$FINDINGS_DIR"/*.json)
shopt -u nullglob
if [ "${#jsons[@]}" -gt 0 ]; then
  echo "=== 评测（路径 b：分析器 → 归一化 findings → eval）==="
  python3 "$ROOT/tools/eval.py" run "$FINDINGS_DIR" "${EVAL_ARGS[@]}"
else
  echo "[warn] $FINDINGS_DIR 下无归一化 findings JSON，跳过评分。"
  echo "       请在包装命令里把每例一个 <case_id>.json（格式见 schema/findings.schema.json）"
  echo "       写入 \$FINDINGS_DIR；已有产物时可改走 --findings-dir 直接评分。"
fi
