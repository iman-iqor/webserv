#!/usr/bin/bin/python3
import os

def main():
    # Every line must end with a newline. The Status header is consumed by your server.
    print("Status: 200 OK")
    print("Content-Type: text/html; charset=utf-8")
    
    # 4. Print the CRITICAL Empty Line
    # This separates the headers from the body payload
    print("")

    # 5. Print the Response Body
    html_content = f"""<!DOCTYPE html>
<html>
<head>
    <title>CGI GET Response</title>
    <style>
        body {{ font-family: sans-serif; margin: 40px; background-color: #f4f4f9; color: #333; }}
        .card {{ background: white; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 400px; }}
        h1 {{ color: #007acc; }}
    </style>
</head>
<body>
    <div class="card">
        <h1>Hello, {user_name}!</h1>
        <p><strong>Role detected:</strong> {user_role}</p>
        <p><strong>Raw Query String:</strong> <code>{query_string if query_string else 'None'}</code></p>
    </div>
</body>
</html>"""
    
    print(html_content)

if __name__ == "__main__":
    main()