#!/usr/bin/env python3
"""Send a JPEG to EmoBot Qt companion via UDP (127.0.0.1:5005).

Tests fragmented delivery: splits payload into small datagrams so Qt must reassemble SOI..EOI.

Usage:
  python udp_jpeg_test_sender.py /path/to/image.jpg
"""
import argparse
import socket
import sys
import time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("jpeg_path", help="Path to a .jpg file")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=5005)
    ap.add_argument("--chunk", type=int, default=1200, help="bytes per UDP datagram")
    ap.add_argument("--loop", action="store_true", help="resend periodically")
    args = ap.parse_args()

    data = open(args.jpeg_path, "rb").read()
    if not data.startswith(b"\xff\xd8"):
        print("File does not look like JPEG (missing SOI)", file=sys.stderr)
        sys.exit(1)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    host_port = (args.host, args.port)
    try:
        while True:
            for i in range(0, len(data), args.chunk):
                sock.sendto(data[i : i + args.chunk], host_port)
                time.sleep(0.001)
            print(f"sent {len(data)} bytes in {(len(data) + args.chunk - 1) // args.chunk} dgrams -> {host_port}")
            if not args.loop:
                break
            time.sleep(1.0)
    finally:
        sock.close()


if __name__ == "__main__":
    main()
