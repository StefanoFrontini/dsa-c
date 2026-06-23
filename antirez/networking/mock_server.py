import socket
import time

# Configurazione del socket sulla porta locale 8080
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', 8080))
s.listen(1)
print("🚀 Mock Server in ascolto su http://127.0.0.1:8080 ...")

while True:
    conn, addr = s.accept()
    request = conn.recv(1024) # Consumiamo la richiesta GET del tuo client C

    # 1. Inviamo gli header HTTP standard
    conn.sendall(b"HTTP/1.1 200 OK\r\n"
                 b"Content-Type: text/plain\r\n"
                 b"Transfer-Encoding: chunked\r\n"
                 b"Connection: close\r\n\r\n")

    # 2. Invia Chunk 1 (contiene un carattere \r interno molto bastardo per i parser!)
    print("Inviando il primo chunk...")
    conn.sendall(b"5\r\nWi\rki\r\n")
    time.sleep(0.8) # Simula il ritardo della rete (es. galleria della metro)

    # 3. Invia Chunk 2
    print("Inviando il secondo chunk...")
    conn.sendall(b"5\r\npedia\r\n")
    time.sleep(0.8)

    # 4. Invia il Chunk finale di chiusura (0)
    print("Inviando il chunk finale.")
    conn.sendall(b"0\r\n\r\n")

    conn.close()