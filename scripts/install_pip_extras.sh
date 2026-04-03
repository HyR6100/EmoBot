#!/usr/bin/env bash
# 在 Isaac Sim 使用的 conda 环境中安装 edge-tts（TTS）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ENV="${EMOBOT_CONDA_ENV:-isaacsim}"

if command -v conda >/dev/null 2>&1; then
  conda run -n "$ENV" pip install -U pip
  conda run -n "$ENV" pip install -r "$ROOT/requirements-pip-extras.txt"
  echo "已在 conda 环境 [$ENV] 中安装 pip 额外依赖。"
else
  echo "未找到 conda，请手动在目标环境中执行:"
  echo "  pip install -r $ROOT/requirements-pip-extras.txt"
  exit 1
fi
