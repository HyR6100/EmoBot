# Blender Text 编辑器中运行；与 Qt CompanionBackend 监听端口 5005 一致。
# EmoBot 仓库备份副本——以你工程里的版本为准。

import bpy
import socket
import os
import tempfile

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
tmp_path = os.path.join(tempfile.gettempdir(), "emo_fixed.jpg")


def send_frame():
    try:
        scene = bpy.context.scene

        # 1. 强制设定超低分辨率（物理限制大小）
        scene.render.resolution_x = 480
        scene.render.resolution_y = 270
        scene.render.resolution_percentage = 100

        scene.render.image_settings.file_format = "JPEG"
        scene.render.image_settings.quality = 25

        # 2. 执行渲染（不写盘，直接存内存缓冲区）
        # 如果这个操作太慢，就把渲染引擎改成 Workbench
        bpy.ops.render.render(write_still=False)

        # 3. 将渲染结果保存到临时文件
        bpy.data.images["Render Result"].save_render(tmp_path)

        if os.path.exists(tmp_path):
            with open(tmp_path, "rb") as f:
                data = f.read()
                # 只要这个数字小于 65000 字节，就能通！
                if len(data) < 65000:
                    sock.sendto(data, (UDP_IP, UDP_PORT))
                    print(f"【发送成功】大小: {len(data)} 字节 (480x270)")
                else:
                    print(f"【依然失败】当前 {len(data)} 字节，请尝试调低 Quality 到 10")

        return 0.1
    except Exception as e:
        print(f"错误: {e}")
        return 1.0


# 注册
if hasattr(bpy.app, "timers"):
    try:
        bpy.app.timers.unregister(send_frame)
    except Exception:
        pass
    bpy.app.timers.register(send_frame)
    print("--- 强制分辨率模式已启动 ---")
