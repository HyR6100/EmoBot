import os
import socket
import json

EMOTION_TO_ACTION = {
    "happy":   "h1_happy.pt",
    "sad":     "h1_sad.pt",
    "clingy":  "h1_clingy.pt",
    "excited": "h1_excited.pt",
}

GESTURE_TO_DESC = {
    "nod":         "点头",
    "walk_closer": "走近",
    "turn_away":   "背过身",
    "stand_still": "静止",
}

BASE_MODEL = "h1_base_walk.pt"
ROBOT_SERVER = ('127.0.0.1', 12345)

def trigger_action(emotion: str, gesture: str):
    model = EMOTION_TO_ACTION.get(emotion, BASE_MODEL)
    gesture_desc = GESTURE_TO_DESC.get(gesture, "静止")
    
    model_path = os.path.expanduser(f"~/EmoBot/models/{model}")
    if not os.path.exists(model_path):
        model = BASE_MODEL

    print(f"[动作触发] 情绪:{emotion} → 模型:{model} | 手势:{gesture_desc}")

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(1)
        s.connect(ROBOT_SERVER)
        s.sendall(json.dumps({"emotion": emotion, "gesture": gesture}).encode())
        s.close()
    except:
        pass

    notify_blender(emotion, gesture)
    return model, gesture_desc

def notify_blender(emotion: str, gesture: str):
    """发送情绪给Blender"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(1)
        s.connect(('127.0.0.1', 12346))
        s.sendall(json.dumps({"emotion": emotion, "gesture": gesture}).encode())
        s.close()
    except:
        pass
