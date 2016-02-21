#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <ev.h>

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

void loop() {


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
