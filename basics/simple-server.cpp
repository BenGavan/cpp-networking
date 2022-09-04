#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdlib>
#include <cstdio>
#include <netdb.h>

// c++ 
#include <iostream>
#include <string>

#include <fcntl.h> // for open
#include <unistd.h> // for close

#define BACKLOG 4  // max number of clients in the queue (number of clients waiting to be handled)

int main() {
    std::cout << "Simple server" << std::endl;

    register int s, c;
    socklen_t b;  // type = unsigned int
    struct sockaddr_in sa;
    time_t t;
    struct tm *tm;
    FILE *client;

    s = socket(PF_INET, SOCK_STREAM, 0);
    if (s <= 0) {
        perror("socket");
        return 1;
    }

    bzero(&sa, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(13);

    if (INADDR_ANY) {
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(s, (struct sockaddr *) &sa, sizeof sa) < 0) {
        perror("bind");
        return 2;
    }

    switch (fork()) {
        case -1:
            perror("fork");
            return 3;
            break;
        default:  // Parent process
            std::cout << "Closing original socket";
            std::cout << "\nEnd of parent process" << std::endl;
            close(s);
            return 0;  //  end of parent process
            break;
        case 0:  // Child process
            std::cout << "Begining of Child process" << std::endl;
            break;
    }
    // Process child from here on

    listen(s, BACKLOG);

    for (;;) {
        b = sizeof sa;
        c = accept(s, (struct sockaddr *) &sa, &b);

        if (c < 0) {
            perror("daytime accept");
            return 4;
        }

        client = fdopen(c, "w");
        if (client == NULL) {
            perror("daytime fdopen");
            return 5;
        }

        t = time(NULL);
        if (t == 0) {
            perror("daytimed time");
            return 6;
        }


        tm = gmtime(&t);
        fprintf(client, "%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ\n",
                tm->tm_year + 1900,
                tm->tm_mon + 1,
                tm->tm_mday,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec);

        fprintf(client, "Hey");

        fclose(client);
    }

    return 0;
}
