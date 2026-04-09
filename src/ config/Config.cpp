//oum

server {
    listen 127.0.0.1:8080;
    server_name localhost;
    root /var/www/html;
    client_max_body_size 10M;
    
    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;
    
    location / {
        allow_methods GET POST DELETE;
        index index.html;
        autoindex off;
    }
    
    location /upload {
        allow_methods POST;
        upload_dir /var/uploads;
    }
    
    location /cgi-bin {
        allow_methods GET POST;
        cgi_extension .php;
    }
}

server {
    listen 127.0.0.1:8081;
    server_name api.localhost;
    root /var/www/api;
}