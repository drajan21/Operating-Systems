main_functionality.exe : main_functionality.o mimetypes.o logging.o server.o strategies.o bounded_buffer.o http_response.o http_request.o
	gcc main_functionality.o mimetypes.o logging.o server.o strategies.o bounded_buffer.o http_response.o http_request.o -o main_functionality.exe -pthread

main_functionality.o : main_functionality.c
	gcc -c main_functionality.c

mimetypes.o : mimetypes.c
	gcc -c mimetypes.c

logging.o : logging.c
	gcc -c logging.c

server.o : server.c
	gcc -c server.c

strategies.o : strategies.c
	gcc -c strategies.c

bounded_buffer.o : bounded_buffer.c
	gcc -c bounded_buffer.c

http_response.o : http_response.c
	gcc -c http_response.c

http_request.o : http_request.c
	gcc -c http_request.c
