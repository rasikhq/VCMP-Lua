"""HTTPS server for the smoke test (tests/integration/compose.yml).

Creates a test CA and a server certificate for the name "tls-server" in
/certs (only the CA certificate is meant for the client), then answers
GET <path> with "hello <path>" on port 8443. The container is also reachable
as "tls-wrong", a name the certificate does not carry.
"""
import http.server
import os
import ssl
import subprocess

CERTS = "/certs"


def openssl(*args):
    subprocess.run(["openssl", *args], check=True, capture_output=True)


def make_certificates():
    os.makedirs(CERTS, exist_ok=True)
    key, csr, cert = "/tmp/server.key", "/tmp/server.csr", "/tmp/server.pem"
    ca_key, ca = "/tmp/ca.key", f"{CERTS}/ca.pem"
    openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "2",
            "-subj", "/CN=VCMP-Lua smoke test CA", "-keyout", ca_key, "-out", ca)
    openssl("req", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=tls-server",
            "-keyout", key, "-out", csr)
    with open("/tmp/ext.cnf", "w") as ext:
        ext.write("subjectAltName=DNS:tls-server\n"
                  "basicConstraints=CA:FALSE\n"
                  "keyUsage=digitalSignature,keyEncipherment\n"
                  "extendedKeyUsage=serverAuth\n")
    openssl("x509", "-req", "-in", csr, "-CA", ca, "-CAkey", ca_key, "-CAcreateserial",
            "-days", "2", "-extfile", "/tmp/ext.cnf", "-out", cert)
    os.chmod(ca, 0o644)
    return cert, key


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        body = f"hello {self.path}".encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        pass


class Server(http.server.ThreadingHTTPServer):
    request_queue_size = 256


def main():
    cert, key = make_certificates()
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert, key)
    server = Server(("0.0.0.0", 8443), Handler)
    server.socket = context.wrap_socket(server.socket, server_side=True)
    open(f"{CERTS}/ready", "w").close()
    print("TLS server ready on port 8443", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
