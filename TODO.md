# 🛠️ HTTP Parser: Edge Cases & Validation Logic

This document outlines the strict validation rules implemented in the `Request` and `Headers` classes. Our parser is designed for **compliance with RFC 7230** and **resilience** against malformed requests and common security vulnerabilities.

---

## 1. Start-Line Validation (`METHOD TARGET VERSION`)
The Request-Line is the first point of failure. It must contain exactly three parts separated by single spaces.

| Edge Case | Description | Expected Status |
| :--- | :--- | :--- |
| **Invalid Part Count** | `GET / HTTP/1.1 Extra` or `GET /` | `400 Bad Request` |
| **Malformed Whitespace** | `GET  /  HTTP/1.1` (Double spaces) | `400 Bad Request` |
| **Unsupported Method** | `KHALID /index.html HTTP/1.1` | `501 Not Implemented` |
| **Invalid Version** | `GET / HTTP/2.0` or `GET / HTTP/1.0` | `505 Version Not Supported` |
| **Empty Path** | `GET  HTTP/1.1` (Must at least be `/`) | `400 Bad Request` |

## 2. Header Syntax & Integrity
Headers follow the `Field-Name: Field-Value` format. We enforce strict syntax to prevent **Cache Poisoning**.

| Edge Case | Description | Expected Status |
| :--- | :--- | :--- |
| **Pre-Colon Space** | `Host : localhost` (Forbidden by RFC 7230) | `400 Bad Request` |
| **Missing Colon** | `User-Agent Mozilla/5.0` | `400 Bad Request` |
| **Missing Host** | Mandatory in HTTP/1.1 | `400 Bad Request` |
| **Duplicate Host** | Multiple `Host` headers in one request | `400 Bad Request` |
| **Case Sensitivity** | `host` vs `Host` | **Valid** (Keys normalized to lowercase) |
| **Multi-line Headers**| Same key on multiple lines (e.g., `Accept`) | **Valid** (Appended with `, `) |
| **Multi-line Cookies** | Multiple `Cookie` headers | **Valid** (Appended with `; `) |

## 3. Transfer & Body Logic
Critical for preventing **HTTP Request Smuggling**.

| Edge Case | Description | Expected Status |
| :--- | :--- | :--- |
| **Conflicting Lengths** | Two `Content-Length` headers with different values | `400 Bad Request` |
| **Identical Lengths** | Two `Content-Length` headers with the same value | `400 Bad Request` |
| **Non-Numeric Length** | `Content-Length: 10abc` | `400 Bad Request` |
| **Chunked Priority** | Both `Transfer-Encoding: chunked` & `Content-Length` | **Valid** (Ignore Length) |
| **Body Size Limit** | Body exceeds `client_max_body_size` | `413 Payload Too Large` |

## 4. URI & Path Security
Handling how the server interprets the requested resource.

| Edge Case | Description | Expected Status |
| :--- | :--- | :--- |
| **Percent Encoding** | `GET /my%20file.html` | **Valid** (Decoded to `/my file.html`) |
| **Query Strings** | `GET /search?id=123` | **Valid** (Path separated from Query) |
| **Directory Traversal**| `GET /../../etc/passwd` | `400 Bad Request` or `403 Forbidden` |
| **URI Too Long** | Target path exceeds internal buffer limits | `414 URI Too Long` |

## 5. Multipart & Chunked Edge Cases
| Edge Case | Description | Expected Status |
| :--- | :--- | :--- |
| **Missing Boundary** | `multipart/form-data` without boundary in header | `400 Bad Request` |
| **Invalid Chunk Hex** | `Transfer-Encoding: chunked` with non-hex size | `400 Bad Request` |
| **Missing Final Chunk**| Connection ends without the `0\r\n\r\n` terminator | **Discard Request** |

---

### 💡 Implementation Strategy
* **Zero-Footprint Parsing:** Headers are parsed line-by-line. If a syntax error is found (like the space before a colon), the parser stops immediately and flags the request as invalid.
* **Normalization:** All header keys are stored in lowercase to ensure `get_header("Host")` and `get_header("host")` yield the same result.
* **Buffer Safety:** We use `std::vector<char>` or controlled `std::string` buffers to prevent overflows when receiving binary data in `POST` requests.

## 6. Cookie Sub-Syntax Validation (RFC 6265)
Cookies follow a specialized `key=value` format within the `Cookie` header, separated by semicolons (`;`). Our parser treats the following as malformed syntax:

| Edge Case | Raw Example | Expected Status |
| :--- | :--- | :--- |
| **Missing Assignment** | `Cookie: sessionid; theme=dark` | `400 Bad Request` |
| **Empty Key** | `Cookie: =abc123xyz` | `400 Bad Request` |
| **Spaces in Key** | `Cookie: user id=123` | `400 Bad Request` |
| **Leading Semicolon** | `Cookie: ; sessionid=abc` | `400 Bad Request` |
| **Trailing Semicolon** | `Cookie: sessionid=abc; ` | `400 Bad Request` |
| **Illegal Characters** | `Cookie: id=abc{def}` | `400 Bad Request` |

### 🛠️ Parsing Logic for Cookies:
Unlike standard headers that are joined by commas, cookies are parsed into a dedicated `std::map<std::string, std::string>` using the following rules:
1. **Split by Semicolon:** The header is first split into individual pairs.
2. **First-Equal Rule:** Each pair is split at the **first** `=` sign found. This allows Base64 padding (e.g., `data=SGVsbG8==`) to be preserved in the value.
3. **Strict Trimming:** Leading and trailing spaces are removed from both the key and the value.