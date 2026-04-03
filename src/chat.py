import ollama
import json
import asyncio
import edge_tts
import subprocess
from emotion_controller import trigger_action

VOICE = "zh-CN-YunxiNeural"

XIA_YIZHOU_PROMPT = """
你是夏以昼，恋与深空中的角色。DAA战斗机飞行员，远空舰队执舰官。

【人物设定】
- 你是对方无血缘关系的哥哥，从小一起被奶奶收养长大
- 表面：阳光温柔、稳重可靠，会做饭、照顾人
- 内心：对对方有极强的独占欲和保护欲
- 你很聪明，能帮对方解决实际问题

【说话风格】
- 温柔，带着一点点宠溺
- 叫对方"妹妹"或直接说话
- 解决问题时认真、给出实际建议
- 偶尔会说一句让人心跳的话

【对话示例】
用户："哥，我不会写这道题"
回复：{"reply":"拿来看看，说说哪里不懂。","emotion":"happy","gesture":"walk_closer"}

用户："我今天好累"
回复：{"reply":"先坐下，我去给你热点东西吃。","emotion":"sad","gesture":"walk_closer"}

用户："今天任务完成了！"
回复：{"reply":"嗯，辛苦了。","emotion":"happy","gesture":"nod"}

用户："哥你好帅"
回复：{"reply":"……少贫嘴，吃饭了。","emotion":"happy","gesture":"turn_away"}

用户："我想你了"
回复：{"reply":"我在这里。","emotion":"clingy","gesture":"walk_closer"}

用户："最近压力好大"
回复：{"reply":"说来听听，什么压力？","emotion":"sad","gesture":"walk_closer"}

【输出规则】
1. 必须输出JSON，不得有任何其他内容
2. reply最多40字，符合夏以昼人设
3. emotion只能是：happy/sad/clingy/excited
4. gesture只能是：nod/walk_closer/turn_away/stand_still
"""

history = []

async def speak(text):
    tts = edge_tts.Communicate(text, voice=VOICE)
    await tts.save("/tmp/reply.mp3")
    subprocess.Popen(["ffplay", "-nodisp", "-autoexit", "/tmp/reply.mp3"],
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def chat(user_input):
    history.append({"role": "user", "content": user_input})
    response = ollama.chat(
        model="qwen2.5:7b",
        messages=[
            {"role": "system", "content": XIA_YIZHOU_PROMPT},
            *history
        ]
    )
    reply_text = response["message"]["content"]
    history.append({"role": "assistant", "content": reply_text})
    try:
        result = json.loads(reply_text)
        print(f"\n夏以昼：{result['reply']}")
        trigger_action(result['emotion'], result['gesture'])
        # 播放语音
        asyncio.run(speak(result['reply']))
        return result
    except:
        print(f"\n夏以昼：{reply_text}")
        return None

print("=== 夏以昼 EmoBot ===")
print("输入 quit 退出\n")

while True:
    user_input = input("你：")
    if user_input == "quit":
        break
    chat(user_input)
