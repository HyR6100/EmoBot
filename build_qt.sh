#!/usr/bin/env bash
# 在仓库根目录执行：./build_qt.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/qt_companion"
qmake qt_companion.pro
make -j"$(nproc 2>/dev/null || echo 2)"
echo "完成: $ROOT/qt_companion/EmobotQtCompanion"
