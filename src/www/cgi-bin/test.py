#!/usr/bin/python3
import os

def main():
    # Print CGI headers
    print("Status: 200 OK\r\n", end="")
    print("Content-Type: text/html; charset=utf-8\r\n", end="")
    print("Tkharbi9a: bayna\r\n", end="")
    print("\r\n", end="")  # Empty line to separate headers from body
    
    # Print the response body
    user_name = os.environ.get("SCRIPT_NAME", "Guest")
    query_string = os.environ.get("QUERY_STRING", "")
    request_method = os.environ.get("REQUEST_METHOD", "GET")
    server_software = os.environ.get("SERVER_SOFTWARE", "webserv/1.0")
    
    print("""<!DOCTYPE html>\r
<html>\r
<head>\r
    <title>CGI Test Response</title>\r
    <style>\r
        body { font-family: sans-serif; margin: 40px; background-color: #f4f4f9; color: #333; }\r
        .card { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 600px; }\r
        h1 { color: #007acc; }\r
        table { width: 100%; border-collapse: collapse; }\r
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\r
        th { background-color: #f2f2f2; }\r
    </style>\r
</head>\r
<body>\r
    <div class="card">\r
        <h1>✅ CGI Script Executed Successfully!</h1>\r
        <p>This page was served by a Python CGI script running on your webserv server.</p>\r
        \r
        <h2>Environment Variables:</h2>\r
        <table>\r
            <tr><th>Variable</th><th>Value</th></tr>\r
            <tr><td>REQUEST_METHOD</td><td>""" + request_method + """</td></tr>\r
            <tr><td>SCRIPT_NAME</td><td>""" + user_name + """</td></tr>\r
            <tr><td>QUERY_STRING</td><td>""" + (query_string if query_string else "(empty)") + """</td></tr>\r
            <tr><td>SERVER_SOFTWARE</td><td>""" + server_software + """</td></tr>\r
        </table>\r
        \r
        <hr>\r
        <p><em>If you can see this page, CGI support is working correctly!</em></p>\r
    </div>\r
</body>\r
</html>\r\n""", end="")

if __name__ == "__main__":
    main()
