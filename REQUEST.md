# HTTP/1.1 Request Parsing Summary (RFC 7230)

## 1. Message Structure

HTTP-message =
    start-line
    *(header-field CRLF)
    CRLF
    [message-body]

- Message = start-line + headers + empty line + optional body
- Headers end with CRLF CRLF
- Body is optional

---

## 2. Parsing Strategy

Steps:
1. Read start-line
2. Read headers until empty line
3. Determine if a body exists
4. If yes → read body accordingly

Rules:
- Parse as raw bytes (octets), NOT Unicode
- Do not rely on partial/streamed delivery
- Whitespace before first header:
  - MUST reject OR ignore lines

---

## 3. Start Line

start-line = request-line | status-line

Request-Line:
request-line = method SP request-target SP HTTP-version CRLF

Example:
GET /index.html HTTP/1.1

Rules:
- Exactly 3 parts separated by spaces
- Method is case-sensitive
- No spaces inside components

Errors:
- Invalid → 400 Bad Request or 301
- Unknown method → 501
- URI too long → 414

Limits:
- Support at least 8000 bytes

---

## 4. Headers

header-field = field-name ":" OWS field-value OWS

Example:
Host: example.com

Rules:
- Field-name is case-insensitive
- MUST NOT have space before ":"
  Example (invalid): Host : example.com
- Trim optional whitespace (OWS)

---

## 5. Header Behavior

Order:
- Different headers → order irrelevant
- Same headers → order matters

Duplicates:
- Allowed only if defined as list
- Can be merged with commas

Exception:
- Set-Cookie → cannot be merged

---

## 6. Header Extensibility

- Unlimited headers allowed
- Unknown headers:
  - Proxy → MUST forward
  - Server → SHOULD ignore

---

## 7. Obsolete Folding (obs-fold)

Example:
Header: value
        continued

- Deprecated
- MUST NOT be generated
- If received:
  - Reject (400) OR replace with spaces

---

## 8. Header Value Syntax

Token characters:
! # $ % & ' * + - . ^ _ ` | ~ 0-9 A-Z a-z

Quoted string:
"value"

Escape:
\"  \\

Comments:
(value)

---

## 9. Message Body

message-body = *OCTET

Present if:
- Content-Length
- OR Transfer-Encoding

---

## 10. Content-Length

Example:
Content-Length: 123

Rules:
- Defines exact body size
- MUST NOT be used with Transfer-Encoding
- Value must be >= 0

Edge Cases:
- Multiple values:
  - Same → OK
  - Different → ERROR (400)

---

## 11. Transfer-Encoding

Example:
Transfer-Encoding: gzip, chunked

Rules:
- MUST support chunked
- chunked MUST be last
- MUST NOT apply chunked twice

Not allowed in:
- 1xx responses
- 204 responses
- CONNECT (2xx)

---

## 12. Message Body Length (Priority Order)

1. No Body:
- HEAD response
- 1xx, 204, 304

2. CONNECT (2xx):
- Switch to tunnel
- Ignore body headers

3. Transfer-Encoding:
- chunked → read chunks
- not chunked:
  - Response → read until connection close
  - Request → ERROR (400)

4. Transfer-Encoding + Content-Length:
- Transfer-Encoding overrides
- Remove Content-Length
- Possible attack

5. Invalid Content-Length:
- Different values → ERROR

6. Valid Content-Length:
- Read exactly N bytes

7. Request fallback:
- No headers → body = 0

8. Response fallback:
- Read until connection close

---

## 13. Errors & Security

MUST reject:
- Space before ":"
- Invalid request-line
- Invalid Content-Length
- Unknown Transfer-Encoding → 501
- Request smuggling patterns

---

## 14. Incomplete Messages

Request:
- Server MAY send error

Response:
- Client MUST mark incomplete

Cases:
- Chunked → missing last chunk
- Content-Length → fewer bytes

---

## 15. Robustness Rules

Accept:
- Ignore empty lines before request-line
- Accept LF instead of CRLF
- Accept flexible whitespace

Warning:
- Can introduce security issues

---

## 16. Parser Design (42 Webserv)

Step 1: Parse request line
- method
- target
- version

Step 2: Parse headers
- Until CRLF CRLF
- Store in map

Step 3: Validate headers
- No space before colon
- Handle duplicates
- Trim whitespace

Step 4: Detect body
- Check:
  - Transfer-Encoding
  - Content-Length

Step 5: Read body
- Follow rules above

---

## 17. Minimum Requirements for 42

You MUST support:

- Request line parsing
- Headers parsing
- Content-Length
- Chunked encoding
- CRLF handling
- Error handling (400, 414, 501)