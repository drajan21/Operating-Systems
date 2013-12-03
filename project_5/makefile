main.exe : main.o http_request.o server.o strategies.o http_response.o bounded_buffer.o logging.o
	gcc main.o http_request.o server.o strategies.o http_response.o bounded_buffer.o logging.o -o main.exe -pthread

main.o : main.c
	gcc -c main.c

http_request.o : http_request.c
	gcc -c http_request.c

server.o : server.c
	gcc -c server.c

strategies.o : strategies.c
	gcc -c strategies.c

http_response.o : http_response.c
	gcc -c http_response.c


bounded_buffer.o : bounded_buffer.c
	gcc -c bounded_buffer.c

logging.o : logging.c
	gcc -c logging.c




