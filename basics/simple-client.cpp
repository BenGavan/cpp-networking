// #include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// #include <std.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstdlib>
#include <cstdio>

#include <netdb.h>

#include <iostream>
#include <string>

#include <fcntl.h> // for open
#include <unistd.h> // for close


int main() {
    std::cout << "simple daytime client" << std::endl;

    register int s;
    register int bytes;
    struct sockaddr_in sa;
    struct hostent *he;
    char buffer[BUFSIZ+1];
    char* host;

    s = socket(PF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 1;
    }

    bzero(&sa, sizeof sa);

    sa.sin_family = AF_INET;

    struct servent *se;
    se = getservbyname("daytime", "tcp");
    if (se == NULL) {
        fprintf(stderr, "Cannot determine which port to use.\n");
        return 7;
    }
    sa.sin_port = se->s_port;
    // sa.sin_port = htons(13);
// sa.sin_addr.s_addr = htonl((((((192 << 8) | 43) << 8) | 244) << 8) | 18);

    host = "time.nist.gov";
    he = gethostbyname(host);
    if (he == NULL) {
        std::cout << "Invalid host name" << std::endl;
        herror(host);
        return 2;
    }

    std::cout << "host name: " << he->h_name;
    std::cout << "\nhost address type: " << he->h_addrtype;
    std::cout << "\nhost address length: " << he->h_length;
    std::cout << std::endl;

    bcopy(he->h_addr_list[0], &sa.sin_addr, he->h_length);

    std::cout << "s_addr: " << sa.sin_addr.s_addr << std::endl;
    std::cout << "Connecting to socket" << std::endl;

    if (connect(s, (struct sockaddr *) &sa, sizeof sa) < 0) {
        perror("connect");
        close(s);
        return 2;
    }
 
    std::cout << "Reading data" << std::endl;

    while ((bytes = read(s, buffer, BUFSIZ) > 0)) {
        write(1, buffer, bytes);
    }

    std::cout << buffer << std::endl;

    close(s);

    return 0;
}