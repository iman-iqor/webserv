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
print("Content-Type: text/html")
print()  # blank line required between headers and body
print(f"""<!DOCTYPE html>
<html>
<body>
    <h1>POST Request Received</h1>
    <h2>Raw Body:</h2>
    <p>{body}</p>
    <h2>Parsed Params:</h2>
    <ul>
        {"".join(f"<li>{k} = {v}</li>" for k, v in params.items())}
    </ul>
    <h2>Environment:</h2>
    <ul>
        <li>METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}</li>
        <li>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'N/A')}</li>
        <li>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'N/A')}</li>
    </ul>
</body>
</html>""")