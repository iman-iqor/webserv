#!/usr/bin/python3
import os

def main():
    # Print CGI headers
    print("Status: 200 OK")
    print("Content-Type: text/html; charset=utf-8")
    print("")  # Empty line to separate headers from body
    
    # Print the response body
    user_name = os.environ.get("SCRIPT_NAME", "Guest")
    query_string = os.environ.get("QUERY_STRING", "")
    request_method = os.environ.get("REQUEST_METHOD", "GET")
    server_software = os.environ.get("SERVER_SOFTWARE", "webserv/1.0")
    
    print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Test Response</title>
    <style>
        body { font-family: sans-serif; margin: 40px; background-color: #f4f4f9; color: #333; }
        .card { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 600px; }
        h1 { color: #007acc; }
        table { width: 100%; border-collapse: collapse; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <div class="card">
        <h1>✅ CGI Script Executed Successfully!</h1>
        <p>This page was served by a Python CGI script running on your webserv server.</p>
        
        <h2>Environment Variables:</h2>
        <table>
            <tr><th>Variable</th><th>Value</th></tr>
            <tr><td>REQUEST_METHOD</td><td>""" + request_method + """</td></tr>
            <tr><td>SCRIPT_NAME</td><td>""" + user_name + """</td></tr>
            <tr><td>QUERY_STRING</td><td>""" + (query_string if query_string else "(empty)") + """</td></tr>
            <tr><td>SERVER_SOFTWARE</td><td>""" + server_software + """</td></tr>
        </table>
        
        <hr>
        <p><em>If you can see this page, CGI support is working correctly!</em></p>
    </div>
</body>
</html>""")

if __name__ == "__main__":
    main()
