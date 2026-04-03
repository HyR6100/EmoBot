#!/usr/bin/env bash
# 在已激活 isaacsim（或你的 Isaac 环境）的终端中运行 Isaac + TCP:12345 + UDP:9999
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ -f "$ROOT/scripts/env.local.sh" ]]; then
  # shellcheck source=/dev/null
  source "$ROOT/scripts/env.local.sh"
fi
export EMOBOT_ROOT="${EMOBOT_ROOT:-$ROOT}"
cd "$ROOT"

ENV="${EMOBOT_CONDA_ENV:-isaacsim}"
if command -v conda >/dev/null 2>&1; then
  exec conda run -n "$ENV" --no-capture-output python "$ROOT/server/isaac_controller.py"
else
  echo "未找到 conda，假设当前 shell 已激活 Isaac 环境："
  exec python "$ROOT/server/isaac_controller.py"
fi
