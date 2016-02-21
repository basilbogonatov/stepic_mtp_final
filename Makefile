all: http_parser.o
	gcc main.c http_parser.o -o final -lev -lrt
http_parser.o: http_parser.c
	gcc -c http_parser.c
