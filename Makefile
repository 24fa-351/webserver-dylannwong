web_server: web_server.c http_message.c request_endpoints.c
	gcc -o web_server web_server.c http_message.c request_endpoints.c

clean:
	rm web_server