# EmoBot 环境变量示例。复制为本机文件后按需修改路径：
#   cp scripts/env.example.sh scripts/env.local.sh
#   nano scripts/env.local.sh
# 每个新终端启动链路前执行：
#   source "$(dirname "$0")/scripts/env.local.sh"   # 若在仓库根目录
# 或：
#   source /你的路径/EmoBot/scripts/env.local.sh

export EMOBOT_ROOT="${EMOBOT_ROOT:-$HOME/EmoBot}"

# Isaac Lab 源码根目录（与 NVIDIA 文档一致）
export ISAACLAB_ROOT="${ISAACLAB_ROOT:-$HOME/IsaacLab}"

# Qt 调用 TTS 时使用的 Python（须已 pip 安装 edge-tts）
export EMOBOT_PYTHON="${EMOBOT_PYTHON:-$HOME/miniconda3/envs/isaacsim/bin/python}"

# Isaac 侧 H1  walking 策略权重（.pt）；默认指向仓库内 models/
export EMOBOT_H1_CHECKPOINT="${EMOBOT_H1_CHECKPOINT:-$EMOBOT_ROOT/models/h1_base_walk.pt}"

# Isaac 场景里引用的角色 glb（与仓库内路径或你的资产一致）
export EMOBOT_VRM_GLB="${EMOBOT_VRM_GLB:-$EMOBOT_ROOT/xiayizhou.glb}"
