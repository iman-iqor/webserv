#!/usr/bin/python3
# import cgi
import os
import sys

# 1. THE HEADERS
# Your server needs to read these from the pipe first.
# Note the double newline at the end!
print("Content-type: text/html")
print("Status: 200 OK")
print("")

# 2. THE BODY
print("<html><head><title>CGI Test</title></head><body>")
print("<h1>Hello from the CGI script!</h1>")

# Displaying Environment Variables (Passed by your server)
print("<h3>Environment Context:</h3>")
print(f"<ul>")
print(f"<li><b>Request Method:</b> {os.environ.get('REQUEST_METHOD')}</li>")
print(f"<li><b>Script Name:</b> {os.environ.get('SCRIPT_NAME')}</li>")
print(f"</ul>")

# 3. HANDLING DATA (Form input)
# This will read from STDIN if the method is POST
# form = cgi.FieldStorage()
# user_name = form.getvalue('name', 'Stranger')

# print(f"<h2>Welcome, {user_name}!</h2>")

# If this was a POST, show the raw content length
if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    print(f"<p>I received a POST body of {content_length} bytes.</p>")

print("</body></html>")