#!/usr/bin/env python3

import argparse
import io
import queue
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer


JPEG_SOI = b"\xff\xd8"
JPEG_EOI = b"\xff\xd9"


def find_jpegs_from_buffer(buf: bytearray):
    frames = []
    while True:
        start = buf.find(JPEG_SOI)
        if start < 0:
            # keep last few bytes in case the next chunk completes SOI
            if len(buf) > 2:
                del buf[:-2]
            break

        end = buf.find(JPEG_EOI, start + 2)
        if end < 0:
            # wait for more bytes
            if start > 0:
                del buf[:start]
            break

        frames.append(bytes(buf[start : end + 2]))
        del buf[: end + 2]
    return frames


class LatestFrameQueue:
    def __init__(self):
        self.q = queue.Queue(maxsize=1)

    def put_latest(self, frame: bytes):
        try:
            while True:
                self.q.get_nowait()
        except queue.Empty:
            pass
        self.q.put_nowait(frame)

    def get(self):
        return self.q.get()


def ffmpeg_capture_stdout(
    display: str,
    x: int,
    y: int,
    width: int,
    height: int,
    fps: int,
    q: LatestFrameQueue,
):
    # ffmpeg x11grab gives each frame as separate JPEG images when using image2pipe + mjpeg codec.
    input_str = f"{display}+{x},{y}"
    cmd = [
        "ffmpeg",
        "-hide_banner",
        "-loglevel",
        "error",
        "-f",
        "x11grab",
        "-video_size",
        f"{width}x{height}",
        "-framerate",
        str(fps),
        "-i",
        input_str,
        "-f",
        "image2pipe",
        "-vcodec",
        "mjpeg",
        "-q:v",
        "5",
        "-",
    ]

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, bufsize=0)
    buf = bytearray()

    try:
        assert proc.stdout is not None
        while True:
            chunk = proc.stdout.read(4096)
            if not chunk:
                time.sleep(0.01)
                continue
            buf.extend(chunk)
            for frame in find_jpegs_from_buffer(buf):
                if frame:
                    q.put_latest(frame)
    finally:
        try:
            proc.kill()
        except Exception:
            pass


class MjpegHandler(BaseHTTPRequestHandler):
    # shared singleton (set by server)
    frame_queue: LatestFrameQueue = None
    boundary = "frame"

    def do_GET(self):
        if self.path not in ("/", "/stream"):
            self.send_error(404)
            return

        self.send_response(200)
        self.send_header("Age", "0")
        self.send_header("Cache-Control", "no-cache, private")
        self.send_header("Pragma", "no-cache")
        self.send_header("Content-Type", f"multipart/x-mixed-replace; boundary={self.boundary}")
        self.end_headers()

        # Write frames until client disconnects.
        while True:
            frame = self.frame_queue.get()
            try:
                self.wfile.write(f"--{self.boundary}\r\n".encode("ascii"))
                self.wfile.write(b"Content-Type: image/jpeg\r\n")
                self.wfile.write(f"Content-Length: {len(frame)}\r\n\r\n".encode("ascii"))
                self.wfile.write(frame)
                self.wfile.write(b"\r\n")
                self.wfile.flush()
            except BrokenPipeError:
                break

    def log_message(self, format, *args):
        # quiet
        return


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8090)
    ap.add_argument("--display", default=":0.0")
    ap.add_argument("--x", type=int, default=0)
    ap.add_argument("--y", type=int, default=0)
    ap.add_argument("--width", type=int, default=1280)
    ap.add_argument("--height", type=int, default=720)
    ap.add_argument("--fps", type=int, default=12)
    args = ap.parse_args()

    q = LatestFrameQueue()
    MjpegHandler.frame_queue = q

    t = threading.Thread(
        target=ffmpeg_capture_stdout,
        args=(args.display, args.x, args.y, args.width, args.height, args.fps, q),
        daemon=True,
    )
    t.start()

    server = HTTPServer(("127.0.0.1", args.port), MjpegHandler)
    print(f"[MJPEG] serving http://127.0.0.1:{args.port}/stream  (display={args.display}+{args.x},{args.y} {args.width}x{args.height} fps={args.fps})")
    server.serve_forever()


if __name__ == "__main__":
    main()

