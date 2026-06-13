#!/usr/bin/env python3
import sys
import os

# Read POST body from stdin
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else "?"

# Parse simple key=value pairs
params = {}
if os.environ.get('QUERY_STRING', 0):
    pairs = os.environ.get('QUERY_STRING', 0).split('&')
    for pair in pairs:
        if '=' in pair:
            key, value = pair.split('=', 1)
            params[key] = value

# CGI response
print("Content-Type: text/html\r\nStatus: 200 OK\r\n\r\n", end='')  # HTTP header
# 1. Compute the list item blocks outside of the f-string expression
parsed_params_html = "\r\n".join(f"\t<li>{k} = {v}</li>" for k, v in params.items())
env_html = "\r\n".join(f"\t<li>{k} = {v}</li>" for k, v in os.environ.items())

# 2. Print using clean, simple variable placeholders (No backslashes inside brackets!)
print(f"""<!DOCTYPE html>\r
<html>\r
<body>\r
    <h1>POST Request Received</h1>\r
    <h2>Raw Body:</h2>\r
    <p>{body}</p>\r
    <h2>Parsed Params:</h2>\r
    <ul>\r
{parsed_params_html}
    </ul>\r
    <h2>Environment:</h2>\r
    <ul>\r
{env_html}
    </ul>\r
</body>\r
</html>""", end='')