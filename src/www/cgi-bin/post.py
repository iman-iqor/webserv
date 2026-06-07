#!/usr/bin/env python3

import os
import sys

# Read Content-Length sent by the server
content_length = int(os.environ.get("CONTENT_LENGTH", 0))

# Read POST body from stdin
body = sys.stdin.read(content_length)

# Send CGI response
print("Content-Type: text/plain")
print()

print("CGI POST Request Received")
print("-------------------------")
print(f"Method: {os.environ.get('REQUEST_METHOD')}")
print(f"Content-Length: {content_length}")
print()
print("Body:")
print(body)