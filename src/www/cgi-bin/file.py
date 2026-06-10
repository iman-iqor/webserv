#!/usr/bin/env python3
import sys
import os

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""


# with open("output.txt", "w") as f:
#     f.write(f"Content-Length: {content_length}\n")
#     f.write(f"{body}\n")

print("Content-Type: text/html\r\nStatus: 200 OK\r\n\r\n", end='')  # HTTP header