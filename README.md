# HttpProxy

A minimal HTTP proxy server written in C++, using RAII for socket safety and custom URL parsing. The proxy listens on port 8080, accepts one client connection, parses an HTTP request, and forwards it to the remote host, returning the response back to the client.

## Features (Current Status)

- Listens on port 8080 and accepts a single HTTP connection.
- Parses incoming HTTP requests (extracts request line, verb, URL, and HTTP version).
- Custom URL parsing with basic support for extracting scheme, host, and rest-of-URL.
- Establishes a connection to the remote host (on port 80 by default) and forwards the client request.
- Receives response from the remote server and returns it to the client.
- Uses RAII wrappers for socket resource management.
- Minimal error handling and diagnostic output.
- Build system: CMake.

## Building and Running

1. **Configure and build:**
   ```sh
    cmake -B build
    cmake --build build
    ```
2. **Run the proxy:**
   ```sh
    ./build/proxy
    ```
   The proxy will start on port 8080.

## Limitations / What Still Needs Work

- **Single request only:** Currently handles only one HTTP request per client; no loop to handle multiple/persistent connections.
- **URL length:** Only supports URLs up to 1024 bytes; RFC allows up to 2048 bytes.
- **Header handling:** Does not yet strip proxy-specific headers like `Proxy-Connection`.
- **No X-Forwarded-For:** The proxy does not add `X-Forwarded-For` header to forwarded requests.
- **Port parsing:** Custom URL parser does not support explicit port numbers in URLs (`host:port`).
- **No HTTPS support:** Only plain HTTP (TCP port 80) is supported.
- **Validation:** Minimal request and URL validation; should be improved for robustness and security.
- **No configuration options:** Port number, etc. are hard-coded.

## Planned Improvements

- Loop to support multiple/persistent client requests.
- Increase URL buffer to 2048 bytes (or dynamically allocate).
- Parse and forward the `X-Forwarded-For` header.
- Properly parse URLs with explicit ports.
- Remove or sanitize proxy-specific headers before forwarding.
- Add command-line/config options for customization.
- More robust error handling and logging.
- Add tests and CI integration.

---

Contributions and bug reports are welcome!
