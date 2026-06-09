#!/usr/bin/env python3
import sys
import os

# Read POST body from stdin
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

# Parse simple key=value pairs
params = {}
if body:
    pairs = body.split('&')
    for pair in pairs:
        if '=' in pair:
            key, value = pair.split('=', 1)
            params[key] = value

# CGI response
print("Content-Type: text/html\r\nStatus: 200 OK\r\n\r\n", end='')  # HTTP header
print(f"""<!DOCTYPE html>\r
<html>\r
<body>\r
    <h1>POST Request Received</h1>\r
    <h2>Raw Body:</h2>\r
    <p>{body}</p>\r
    <h2>Parsed Params:</h2>\r
    <ul>\r
        {"".join(f"<li>{k} = {v}</li>" for k, v in params.items())}\r
    </ul>\r
    <h2>Environment:</h2>\r
    <ul>\r
        <li>METHOD: {os.environ.get('HTTP_REQUEST_METHOD', 'N/A')}</li>\r
        <li>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'N/A')}</li>\r
        <li>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'N/A')}</li>\r
    </ul>\r
</body>\r
</html>""", end='')
