#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <ev.h>
#include "http_parser.h"

struct server_opts {
	char* host;
	int port;
	char* dir;
} opts;

void init_opts(int argc, char** argv) {
	opts.host = 0;
    	opts.port = 0;
    	opts.dir = 0;

    	int c;
   	opterr = 0;
    	while ((c = getopt (argc, argv, "h:p:d:")) != -1)
        	switch (c) {
            		case 'h':
                		opts.host = optarg;
            		break;
            		case 'p':
                		opts.port = atoi(optarg);
            		break;
            		case 'd':
                		opts.dir = optarg;
            		break;
        	}

    	printf("Current server options: host = %s, port = %d, dir = %s\n", opts.host, opts.port, opts.dir);

    	if (!(opts.host && opts.port && opts.dir)) {
        	printf("Please specify all server options.\n");
        	abort();
    	}
}

typedef struct {
	char buf[1024];
} url_data;

int on_url_cb(http_parser* parser, const char* at, size_t length) {
	strncpy(((url_data*)parser->data)->buf, at, length);
	return 0;	
}

void respond(int client_fd, char* file_path) {
	char* content_buf = file_path; 	
	int c_size = strlen(content_buf);
	char templ[] = "HTTP/1.0 200 OK\r\n"
		       "Content-length: %d\r\n"
		       "Connection: close\r\n"
		       "Content-Type: text/html\r\n"
		       "\r\n"
		       "%s";

	char buf[1024];
	sprintf(buf, templ, c_size, content_buf); 		
	send(client_fd, buf, strlen(buf), MSG_NOSIGNAL);
}

void read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
	char buf[1024];
	
	int r = recv(watcher->fd, buf, sizeof(buf), MSG_NOSIGNAL);
	if (r > 0) {
		http_parser* parser = malloc(sizeof(http_parser));
		http_parser_init(parser, HTTP_REQUEST);

		url_data data;
		bzero(&data, sizeof(data));
		parser->data = &data;
		
		http_parser_settings settings;
		bzero(&settings, sizeof(settings));
		
		settings.on_url = on_url_cb;	
	
		http_parser_execute(parser, &settings, buf, r);
		free(parser);

		respond(watcher->fd, data.buf); 
	}
	
	ev_io_stop(loop, watcher);
	shutdown(watcher->fd, SHUT_RDWR);
	free(watcher);
	return;
} 

void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
	int client_sd = accept(watcher->fd, 0, 0);
	struct ev_io* w_client = (struct ev_io*)malloc(sizeof(struct ev_io));

	ev_io_init(w_client, read_cb, client_sd, EV_READ);
	ev_io_start(loop, w_client);
}

void loop() {
	struct ev_loop* loop = ev_default_loop(0);
	int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(opts.port);
	inet_aton(opts.host, &addr.sin_addr);
	bind(sd, (struct sockaddr*)&addr, sizeof(addr));
	
	listen(sd, SOMAXCONN);

	struct ev_io w_accept;
	ev_io_init(&w_accept, accept_cb, sd, EV_READ);
	ev_io_start(loop, &w_accept);

	while(1) ev_loop(loop, 0);
}

void daemonize() {
    	pid_t pid, sid;
    	if (!fork()) {
        	umask(0);
       		sid = setsid();
        	close(STDIN_FILENO);
        	close(STDOUT_FILENO);
        	close(STDERR_FILENO);

        	loop();
    	} else {
        	exit(0);
    	}
}

int main(int argc, char** argv) {
    	init_opts(argc, argv);

    	printf("Starting server!\n");

	//daemonize();
    	loop();

   	return 0;
}
