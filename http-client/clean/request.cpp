#include "request.h"
#include "response.h"
#include <string>
#include <sstream>
#include <iostream>
// networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>

#include <fcntl.h>  // for open
#include <unistd.h> // for close


std::string Request::toString() const {
    std::stringstream ss;
    // ss << "Raw URL: " << url.raw << std::flush;
    ss << "URL: " << url.toString() << std::endl;
    return ss.str();
}


Response Request::sendRequest() {
    // connect
    int socket = connect_to_url(url.base, url.port);
    if (socket < 0) {
        perror("failed to connect");
        return Response();
    }

    // send data
    std::string header_str = make_header_str(method, url.path, url.base);
    const char* header_chars = header_str.c_str();
    send(socket, header_chars, strlen(header_chars), 0);

    // recieve data
    Response response = read_data_from_socket(socket);
    return response;
}


std::string Request::make_header_str(const method_t &method, const std::string path, const std::string &host) const {
    std::stringstream ss;
    ss << method;
    ss << " " << path;
    ss << " HTTP/1.1";
    ss << "\r\nHost: " << host;
    ss << "\r\n\r\n";
    return ss.str();
}


int Request::connect_to_url(const std::string &host_str, const int &port) const {
    struct sockaddr_in sa;
    struct hostent* he;

    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return -1;
    }

    bzero(&sa, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);  // port on the server we are trying to connect to 

    const char* host = host_str.c_str();
    he = gethostbyname(host);
    if (he == NULL) {
        std::cout << "Invalid host name" << std::endl;
        herror(host);
        return -2;
    }

    bcopy(he->h_addr_list[0], &sa.sin_addr, he->h_length);

    if (connect(s, (struct sockaddr *) &sa, sizeof sa) < 0) {
        perror("connect");
        close(s);
        return -2;
    }
    return s;
}


Response Request::read_data_from_socket(const int &socket) {
    int nbytes_total = 0;
    char buffer[BUFSIZ];

    Response response{};
    std::string remaining_bytes;
    bool is_end_of_response = false;

    while (!is_end_of_response) {
        
        nbytes_total = recv(socket, buffer, sizeof(buffer),0);

        // parse response
        std::string h_str(buffer, nbytes_total);
        is_end_of_response = response.parseResponse(h_str, response, remaining_bytes);
    }

    response.did_connect = true;

    std::cout << "Finished\n";
    return response;
}


