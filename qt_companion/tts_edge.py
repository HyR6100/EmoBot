import argparse
import asyncio

import edge_tts


async def run(text: str, out_path: str, voice: str):
    communicate = edge_tts.Communicate(text=text, voice=voice)
    await communicate.save(out_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--text", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--voice", default="zh-CN-YunxiNeural")
    args = parser.parse_args()

    asyncio.run(run(args.text, args.out, args.voice))


if __name__ == "__main__":
    main()

