#!/usr/bin/python3
import os

def main():
    # Print CGI headers
    print("Status: 200 OK\r\n", end="")
    print("Content-Type: text/html; charset=utf-8\r\n", end="")
    print("Tkharbi9a: bayna\r\n", end="")
    print("\r\n", end="")  # Empty line to separate headers from body
    
    # Print the response body
    # user_name = os.environ.get("HTTP_SCRIPT_NAME")
    # query_string = os.environ.get("HTTP_QUERY_STRING")
    # request_method = os.environ.get("HTTP_REQUEST_METHOD")
    # server_name = os.environ.get("HTTP_SERVER_NAME")
    # server_port = os.environ.get("HTTP_SERVER_PORT")
    # server_protocol = os.environ.get("HTTP_SERVER_PROTOCOL")
    # server_software = os.environ.get("HTTP_SERVER_SOFTWARE")
    # gateway_interface = os.environ.get("HTTP_GATEWAY_INTERFACE")

    # get the rest of the environment variables
    env_vars = {key: value for key, value in os.environ.items()}
    
    # add all the environment variables to the response body
    env_html = "".join(f"<tr><td>{key}</td><td>{value}</td></tr>" for key, value in env_vars.items())
    # make the table with scroll
    print("""<!DOCTYPE html>\r
<html>\r
<head>\r
    <title>CGI Test Response</title>\r
    <style>\r
        body { font-family: sans-serif; margin: 40px; background-color: #f4f4f9; color: #333; }\r
        .card { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 600px; overflow-x: auto; }\r
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
            """ + env_html + """
        </table>\r
        \r
        <hr>\r
        <p><em>If you can see this page, CGI support is working correctly!</em></p>\r
    </div>\r
</body>\r
</html>\r\n""", end="")

if __name__ == "__main__":
    main()
