import socket
import json
import threading

class RobotServer:
    def __init__(self, host='127.0.0.1', port=12345):
        self.host = host
        self.port = port
        self.current_emotion = "happy"
        self.current_gesture = "stand_still"
        
    def start(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((self.host, self.port))
        server.listen(1)
        print(f"[Robot Server] 监听 {self.host}:{self.port}")
        
        while True:
            conn, addr = server.accept()
            threading.Thread(target=self.handle_client, args=(conn,)).start()
    
    def handle_client(self, conn):
        with conn:
            data = conn.recv(1024).decode()
            if data:
                try:
                    msg = json.loads(data)
                    self.current_emotion = msg.get("emotion", "happy")
                    self.current_gesture = msg.get("gesture", "stand_still")
                    print(f"[Robot Server] 情绪={self.current_emotion} | 手势={self.current_gesture}")
                    conn.sendall(b"OK")
                except:
                    conn.sendall(b"ERROR")

if __name__ == "__main__":
    srv = RobotServer()
    srv.start()
