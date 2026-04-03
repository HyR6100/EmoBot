import socket
import json
import threading
import sys
import os
import argparse

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
import time
import gymnasium as gym
from rsl_rl.runners import OnPolicyRunner
import isaaclab_tasks
from isaaclab_tasks.utils import parse_env_cfg, get_checkpoint_path
from isaaclab_rl.rsl_rl import RslRlVecEnvWrapper, handle_deprecated_rsl_rl_cfg


# 初始化UDP广播
BLENDER_ADDR = ('127.0.0.1', 9999)
udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

current_emotion = "happy"
CHECKPOINT = os.path.expanduser("~/EmoBot/models/h1_base_walk.pt")

def socket_listener():
    global current_emotion
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', 12345))
    server.listen(1)
    print("[Isaac Controller] Socket监听中 port=12345")
    while True:
        try:
            conn, _ = server.accept()
            data = conn.recv(1024).decode()
            msg = json.loads(data)
            current_emotion = msg.get("emotion", "happy")
            print(f"[Isaac Controller] 切换情绪: {current_emotion}")
            conn.sendall(b"OK")
            conn.close()
        except Exception as e:
            print(f"[Socket Error] {e}")

t = threading.Thread(target=socket_listener, daemon=True)
t.start()

# 加载环境
env_cfg = parse_env_cfg(
    "Isaac-Velocity-Flat-H1-v0",
    device="cuda:0",
    num_envs=1,
)
env_cfg.episode_length_s = 1000.0

env = gym.make("Isaac-Velocity-Flat-H1-v0", cfg=env_cfg)

# 加载agent配置
from isaaclab_tasks.manager_based.locomotion.velocity.config.h1.agents.rsl_rl_ppo_cfg import H1FlatPPORunnerCfg
agent_cfg = H1FlatPPORunnerCfg()
import importlib.metadata as metadata
installed_version = metadata.version("rsl-rl-lib")
agent_cfg = handle_deprecated_rsl_rl_cfg(agent_cfg, installed_version)
env = RslRlVecEnvWrapper(env, clip_actions=agent_cfg.clip_actions)

# 加载模型
runner = OnPolicyRunner(env, agent_cfg.to_dict(), log_dir=None, device="cuda:0")
runner.load(CHECKPOINT)
policy = runner.get_inference_policy(device=env.unwrapped.device)

# 获取机器人
robot = env.unwrapped.scene["robot"]

# 假设你的机器人对象叫 robot 或 articulations
# 如果你使用的是 Isaac SDK/Core:
body_names = robot.body_names
print("-" * 30)
print("Isaac Sim 机器人骨骼顺序 (Internal Body Indices):")
for i, name in enumerate(body_names):
    print(f"Index {i}: {name}")
print("-" * 30)

# 如果你使用的是 Isaac Lab / Orbit:
# print(f"Body names: {self.robot.data.body_names}")

# 加载VRM角色
from isaaclab.sim import SimulationContext
import omni.usd
from pxr import Usd, UsdGeom

stage = omni.usd.get_context().get_stage()
vrm_prim_path = "/World/VRM_Character"
# 添加VRM角色到场景
vrm_ref = stage.DefinePrim(vrm_prim_path, "Xform")
vrm_ref.GetReferences().AddReference("/home/EmoBot/xiayizhou.glb")
print(f"[Isaac Controller] VRM角色已加载: {vrm_prim_path}")

obs, _ = env.reset()

while simulation_app.is_running():
    with torch.inference_mode():
        actions = policy(obs)
    obs, _, _, _ = env.step(actions)

    # 1. 提取物理位姿数据 [num_envs, num_links, 7] -> 取第0个env
    # 前3位是位置 (x,y,z)，后4位是旋转 (qw, qx, qy, qz)
    try:
        poses = robot.data.body_state_w[0, :, :7].detach().cpu().numpy().tolist()
        
        # 2. 打包位姿和当前情绪
        data_to_send = {
            "poses": poses,
            "emotion": current_emotion
        }
        
        # 3. 通过 UDP 快速广播给 Blender
        udp_sock.sendto(json.dumps(data_to_send).encode(), BLENDER_ADDR)
    except Exception as e:
        # 避免仿真初期数据未初始化导致崩溃
        pass

env.close()
simulation_app.close()
