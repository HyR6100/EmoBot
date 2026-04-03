import socket
import json
import threading
import sys
import os
import argparse
import time
import numpy as np

sys.path.append(os.path.expanduser("~/IsaacLab"))
sys.path.append(os.path.expanduser("~/IsaacLab/scripts/reinforcement_learning/rsl_rl"))

from isaaclab.app import AppLauncher

parser = argparse.ArgumentParser()
parser.add_argument("--num_envs", type=int, default=1)
AppLauncher.add_app_launcher_args(parser)
args_cli = parser.parse_args([])

app_launcher = AppLauncher(args_cli)
simulation_app = app_launcher.app

import torch
import gymnasium as gym
from rsl_rl.runners import OnPolicyRunner

import isaaclab_tasks
from isaaclab_tasks.utils import parse_env_cfg
from isaaclab_rl.rsl_rl import RslRlVecEnvWrapper, handle_deprecated_rsl_rl_cfg


# ===============================
# UDP 设置
# ===============================

BLENDER_ADDR = ("127.0.0.1", 9999)

udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)


# ===============================
# 情绪Socket
# ===============================

current_emotion = "happy"

def socket_listener():
    global current_emotion

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", 12345))
    server.listen(1)

    print("[Isaac Controller] Emotion socket ready")

    while True:
        try:
            conn, _ = server.accept()
            data = conn.recv(1024).decode()

            msg = json.loads(data)
            current_emotion = msg.get("emotion", "happy")

            print("[Emotion Switch]", current_emotion)

            conn.sendall(b"OK")
            conn.close()

        except Exception as e:
            print("Socket error:", e)


threading.Thread(target=socket_listener, daemon=True).start()


# ===============================
# 环境
# ===============================

env_cfg = parse_env_cfg(
    "Isaac-Velocity-Flat-H1-v0",
    device="cuda:0",
    num_envs=1,
)

env_cfg.episode_length_s = 1000.0

env = gym.make("Isaac-Velocity-Flat-H1-v0", cfg=env_cfg)


# ===============================
# PPO agent
# ===============================

from isaaclab_tasks.manager_based.locomotion.velocity.config.h1.agents.rsl_rl_ppo_cfg import H1FlatPPORunnerCfg

agent_cfg = H1FlatPPORunnerCfg()

import importlib.metadata as metadata

installed_version = metadata.version("rsl-rl-lib")

agent_cfg = handle_deprecated_rsl_rl_cfg(agent_cfg, installed_version)

env = RslRlVecEnvWrapper(env, clip_actions=agent_cfg.clip_actions)


# ===============================
# 加载模型
# ===============================

CHECKPOINT = os.path.expanduser("~/EmoBot/models/h1_base_walk.pt")

runner = OnPolicyRunner(
    env,
    agent_cfg.to_dict(),
    log_dir=None,
    device="cuda:0",
)

runner.load(CHECKPOINT)

policy = runner.get_inference_policy(device=env.unwrapped.device)


# ===============================
# 获取机器人
# ===============================

robot = env.unwrapped.scene["robot"]

link_names = robot.data.body_names

print("\n========== H1 Links ==========")

for i, name in enumerate(link_names):
    print(i, name)

print("==============================\n")


obs, _ = env.reset()

last_send = time.time()


# ===============================
# 主循环
# ===============================

while simulation_app.is_running():

    with torch.inference_mode():
        actions = policy(obs)

    obs, _, _, _ = env.step(actions)

    now = time.time()

    # 60Hz发送
    if now - last_send < 1.0 / 60.0:
        continue

    last_send = now

    try:

        poses = robot.data.body_state_w[0, :, :7].detach().cpu().numpy()

        # 获取 root link world position
        root_pos = robot.data.body_state_w[0, 0, :3].detach().cpu().numpy()  # pelvis/world position

        # 假设你保存上一帧位置
        if "prev_root_pos" not in globals():
            prev_root_pos = root_pos.copy()

        # 计算平面方向向量 (XY 平面)
        dir_vec = root_pos[:2] - prev_root_pos[:2]
        if np.linalg.norm(dir_vec) < 1e-4:  # 防止零向量
            dir_vec = np.array([1.0, 0.0])  # 默认朝向 X 方向

        # 归一化
        dir_vec = dir_vec / np.linalg.norm(dir_vec)
        prev_root_pos = root_pos.copy()


        data = {
            "poses": poses.tolist(),
            "root_dir": dir_vec.tolist(), 
            "emotion": current_emotion
        }

        udp_sock.sendto(json.dumps(data).encode(), BLENDER_ADDR)

    except Exception:
        pass


env.close()
simulation_app.close()