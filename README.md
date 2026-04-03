# EmoBot

Dreamy Qt companion UI for Ollama (Qwen), TCP control to Isaac Sim, and live video (HTTP MJPEG or Blender UDP JPEG).

## Qt 程序

```bash
cd qt_companion
qmake qt_companion.pro
make -j4
./EmobotQtCompanion
```

可选：`./EmobotQtCompanion --selftest` 在无界面下测 Ollama 与资源。

## 视频

- **HTTP**：运行 `mjpeg_blender_server.py`（或任意提供 `multipart/x-mixed-replace` JPEG 的服务），默认 URL `http://127.0.0.1:8090/stream`。
- **UDP**：向 `127.0.0.1:5005` 发送 **JPEG 字节流**（可拆成多个 UDP 包，程序按 `FF D8` … `FF D9` 拼完整帧）。自测：`qt_companion/udp_jpeg_test_sender.py 某图.jpg`。

## 依赖

- Qt 5（Widgets + Network）、Ollama、可选 `edge-tts` / `ffplay`（语音）、Isaac Sim 侧 TCP `12345`（与仓库内脚本一致）。
