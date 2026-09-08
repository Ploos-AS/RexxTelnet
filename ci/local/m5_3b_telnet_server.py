#!/usr/bin/env python3
import socket
import sys

host = "127.0.0.1"
port = int(sys.argv[1]) if len(sys.argv) > 1 else 2323

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(1)
    print("M5_3B_SERVER_READY=%s:%d" % (host, port), flush=True)
    conn, addr = server.accept()
    with conn:
        print("M5_3B_ACCEPTED=%s:%s" % addr, flush=True)
        conn.sendall(b"M5.3B READY\r\nlogin: ")
        data = b""
        conn.settimeout(30)
        while b"guest\r\n" not in data and b"guest\n" not in data:
            chunk = conn.recv(1024)
            if not chunk:
                break
            data += chunk
        print("M5_3B_RECEIVED=%r" % data, flush=True)
        if b"guest" in data:
            conn.sendall(b"WELCOME GUEST\r\n")
            print("M5_3B_SENDLINE_OK=1", flush=True)
        else:
            print("M5_3B_SENDLINE_OK=0", flush=True)
