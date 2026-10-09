"""Accepts TCP connections on port 3306 and never sends a byte: a MySQL
server that hangs. The gate's timeout checks connect to it."""
import socket

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("0.0.0.0", 3306))
server.listen(64)
held = []
while True:
    connection, _ = server.accept()
    held.append(connection)
