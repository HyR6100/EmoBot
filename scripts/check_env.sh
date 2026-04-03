#!/usr/bin/env bash
# 部署前自检（不修改系统）
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ERR=0
warn() { echo "[?] $*"; }
ok() { echo "[OK] $*"; }
bad() { echo "[!!] $*"; ERR=1; }

[[ -f "$ROOT/scripts/env.local.sh" ]] && source "$ROOT/scripts/env.local.sh"
export EMOBOT_ROOT="${EMOBOT_ROOT:-$ROOT}"

command -v qmake >/dev/null && ok "qmake: $(command -v qmake)" || bad "未找到 qmake（请安装 qtbase5-dev 或 Qt5）"
command -v make >/dev/null && ok "make" || bad "未找到 make"
command -v ollama >/dev/null && ok "ollama" || warn "未找到 ollama（大模型需自行安装并 pull 模型）"
command -v ffplay >/dev/null && ok "ffplay" || warn "未找到 ffplay（可无，但无法播放 TTS 音频）"

CK="${EMOBOT_H1_CHECKPOINT:-$ROOT/models/h1_base_walk.pt}"
if [[ -f "$CK" ]]; then ok "H1 权重: $CK"; else warn "未找到权重 $CK（Isaac 脚本将不可用，请下载或设置 EMOBOT_H1_CHECKPOINT）"; fi

GLB="${EMOBOT_VRM_GLB:-$ROOT/xiayizhou.glb}"
if [[ -f "$GLB" ]]; then ok "VRM glb: $GLB"; else warn "未找到 $GLB（可在 env.local.sh 里指向你的 glb）"; fi

PY="${EMOBOT_PYTHON:-}"
if [[ -n "$PY" && -x "$PY" ]]; then
  if "$PY" -c "import edge_tts" 2>/dev/null; then ok "edge-tts (python: $PY)"; else bad "Python $PY 未安装 edge-tts，请运行 scripts/install_pip_extras.sh"; fi
elif command -v conda >/dev/null; then
  if conda run -n "${EMOBOT_CONDA_ENV:-isaacsim}" python -c "import edge_tts" 2>/dev/null; then
    ok "edge-tts (conda env ${EMOBOT_CONDA_ENV:-isaacsim})"
  else
    warn "请在 isaacsim 环境中安装: pip install -r requirements-pip-extras.txt"
  fi
else
  warn "未设置 EMOBOT_PYTHON，Qt TTS 将使用系统 python3"
fi

IL="${ISAACLAB_ROOT:-$HOME/IsaacLab}"
if [[ -d "$IL" ]]; then ok "ISAACLAB_ROOT: $IL"; else bad "未找到 Isaac Lab 目录 $IL（请克隆 Isaac Lab 或设置 ISAACLAB_ROOT）"; fi

if [[ $ERR -ne 0 ]]; then
  echo "--- 存在阻塞项，请按 README「部署指南」补齐。 ---"
  exit 1
fi
echo "--- 自检通过（未检测 GPU / Isaac Sim 许可证，以本机为准）。 ---"
