#!/usr/bin/env bash
# 一键：复制环境变量模板 + 安装 pip 额外依赖 + 自检 + 编译 Qt（可选）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ ! -f "$ROOT/scripts/env.local.sh" ]]; then
  cp "$ROOT/scripts/env.example.sh" "$ROOT/scripts/env.local.sh"
  echo "已创建 scripts/env.local.sh ，请编辑其中的路径后重新运行本脚本或 source 该文件。"
fi

echo ">>> 安装 edge-tts 到 conda 环境 [${EMOBOT_CONDA_ENV:-isaacsim}] ..."
bash "$ROOT/scripts/install_pip_extras.sh"

echo ">>> 环境自检 ..."
bash "$ROOT/scripts/check_env.sh" || true

if [[ -t 0 ]] && [[ -t 1 ]]; then
  read -r -p "是否现在编译 Qt 客户端？(y/N) " a
  if [[ "${a:-}" =~ ^[yY]$ ]]; then
    bash "$ROOT/build_qt.sh"
  fi
else
  echo "非交互终端：跳过 Qt 编译。请手动执行 ./build_qt.sh"
fi

echo "完成。请阅读 README「部署指南」按顺序启动 Isaac、Blender、Ollama、Qt。"
